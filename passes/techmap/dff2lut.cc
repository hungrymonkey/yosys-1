/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2012  Clifford Wolf <clifford@clifford.at>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "kernel/yosys.h"
#include "kernel/sigtools.h"
#include "kernel/celltypes.h"
#include "passes/techmap/simplemap.h"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct Dff2lutWorker
{
	const dict<IdString, IdString> &direct_dict;

	RTLIL::Module *module;
	SigMap sigmap;
	CellTypes ct;

	typedef std::pair<RTLIL::Cell*, int> cell_int_t;
	std::map<RTLIL::SigBit, cell_int_t> bit2mux;
	std::vector<RTLIL::Cell*> dff_cells;
	std::map<RTLIL::SigBit, int> bitusers;

	typedef std::map<RTLIL::SigBit, bool> pattern_t;
	typedef std::set<pattern_t> patterns_t;


	Dff2lutWorker(Module *mod, const dict<IdString, IdString> &direct_dict, Cell *cell):
			direct_dict(direct_dict), module(mod), sigmap(mod), ct(module->design)
	{
		


			if( direct_dict.empty() ){
				if (cell->type == "$dff") {
					RTLIL::SigSpec sig_master_a = mod->addWire(NEW_ID, 3);
					RTLIL::SigSpec sig_master_y = mod->addWire(NEW_ID,1);
					RTLIL::SigSpec sig_slave_a = mod->addWire(NEW_ID, 3);
					//RTLIL::SigSpec data = mod->addWire(NEW_ID, GetSize(cell->getPort("\\D")));
					//mod->addDff(NEW_ID, cell->getPort("\\CLK"), tmp, cell->getPort("\\Q"), cell->getParam("\\CLK_POLARITY").as_bool());

					sig_master_a[0]= sig_master_y;
					sig_master_a[1]= cell->getPort("\\D");
					sig_master_a[2]=  cell->getPort("\\C");

					sig_slave_a[0]= sig_master_y; 
					sig_slave_a[1]= cell->getPort("\\Q"); 
					sig_slave_a[2]= cell->getPort("\\C");

	
					RTLIL::Cell *new_cell_master = mod->addLut(NEW_ID, sig_master_a, sig_master_y, RTLIL::Const::from_string("11001010"));
					RTLIL::Cell *new_cell_slave = mod->addLut(NEW_ID, sig_slave_a, cell->getPort("\\Q"), RTLIL::Const::from_string("11001010"));

					log("  created $lut cells %s, %s for %s -> %s.\n", log_id(new_cell_master), log_id(new_cell_slave), log_signal(cell->getPort("\\D")), log_signal(cell->getPort("\\Q")));
					mod->remove(cell);
				}
			}else{
				if (direct_dict.count(cell->type)) {
					RTLIL::SigSpec sig_master_a = mod->addWire(NEW_ID, 3);
					RTLIL::SigSpec sig_master_y = mod->addWire(NEW_ID,1);
					RTLIL::SigSpec sig_slave_a = mod->addWire(NEW_ID, 3);
					//RTLIL::SigSpec data = mod->addWire(NEW_ID, GetSize(cell->getPort("\\D")));
					//mod->addDff(NEW_ID, cell->getPort("\\CLK"), tmp, cell->getPort("\\Q"), cell->getParam("\\CLK_POLARITY").as_bool());

					sig_master_a[0]= sig_master_y;
					sig_master_a[1]= cell->getPort("\\D");
					sig_master_a[2]=  cell->getPort("\\C");

					sig_slave_a[0]= sig_master_y; 
					sig_slave_a[1]= cell->getPort("\\Q"); 
					sig_slave_a[2]= cell->getPort("\\C"); 


					mod->addLut(NEW_ID, sig_master_a, sig_master_y, RTLIL::Const::from_string("11001010"));
					mod->addLut(NEW_ID, sig_slave_a, cell->getPort("\\Q"), RTLIL::Const::from_string("11001010"));
					mod->remove(cell);
				}
			}

	}

};

struct Dff2lutPass : public Pass {
	Dff2lutPass() : Pass("dff2lut", "transform $dff cells to $lut cells") { }
	virtual void help()
	{
		//   |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
		log("\n");
		log("    Dff2lut [options] [selection]\n");
		log("\n");
		log("This pass transforms $dff cells driven by a tree of multiplexers with one or\n");
		log("more feedback paths to $dffe cells. It also works on gate-level cells such as\n");
		log("$_DFF_P_, $_DFF_N_ and $_MUX_.\n");
		log("\n");
		log("    -direct <internal_gate_type> <external_gate_type>\n");
		log("        map directly to external gate type. <internal_gate_type> can\n");
		log("        be any internal gate-level FF cell (except $_DFFE_??_). the\n");
		log("        <external_gate_type> is the cell type name for a cell with an\n");
		log("        identical interface to the <internal_gate_type>, except it\n");
		log("        also has an high-active enable port 'E'.\n");
		log("          Usually <external_gate_type> is an intermediate cell type\n");
		log("        that is then translated to the final type using 'techmap'.\n");
		log("\n");
		log("    -direct-match <pattern>\n");
		log("        like -direct for all DFF cell types matching the expression.\n");
		log("        this will use $__DFFE_* as <external_gate_type> matching the\n");
		log("        internal gate type $_DFF_*_, except for $_DFF_[NP]_, which is\n");
		log("        converted to $_DFFE_[NP]_.\n");
		log("\n");
	}
	virtual void execute(std::vector<std::string> args, RTLIL::Design *design)
	{
		log_header("Executing Dff2lut pass (transform $dff to $lut where applicable).\n");

		bool unmap_mode = false;
		dict<IdString, IdString> direct_dict;

		size_t argidx;
		for (argidx = 1; argidx < args.size(); argidx++) {
			if (args[argidx] == "-unmap") {
				unmap_mode = true;
				continue;
			}
			if (args[argidx] == "-direct" && argidx + 2 < args.size()) {
				string direct_from = RTLIL::escape_id(args[++argidx]);
				string direct_to = RTLIL::escape_id(args[++argidx]);
				direct_dict[direct_from] = direct_to;
				continue;
			}
			if (args[argidx] == "-direct-match" && argidx + 1 < args.size()) {
				bool found_match = false;
				const char *pattern = args[++argidx].c_str();
				if (patmatch(pattern, "$_DFF_P_"  )) found_match = true, direct_dict["$_DFF_P_"  ] = "$_DFFE_PP_";
				if (patmatch(pattern, "$_DFF_N_"  )) found_match = true, direct_dict["$_DFF_N_"  ] = "$_DFFE_NP_";
				if (patmatch(pattern, "$_DFF_NN0_")) found_match = true, direct_dict["$_DFF_NN0_"] = "$__DFFE_NN0";
				if (patmatch(pattern, "$_DFF_NN1_")) found_match = true, direct_dict["$_DFF_NN1_"] = "$__DFFE_NN1";
				if (patmatch(pattern, "$_DFF_NP0_")) found_match = true, direct_dict["$_DFF_NP0_"] = "$__DFFE_NP0";
				if (patmatch(pattern, "$_DFF_NP1_")) found_match = true, direct_dict["$_DFF_NP1_"] = "$__DFFE_NP1";
				if (patmatch(pattern, "$_DFF_PN0_")) found_match = true, direct_dict["$_DFF_PN0_"] = "$__DFFE_PN0";
				if (patmatch(pattern, "$_DFF_PN1_")) found_match = true, direct_dict["$_DFF_PN1_"] = "$__DFFE_PN1";
				if (patmatch(pattern, "$_DFF_PP0_")) found_match = true, direct_dict["$_DFF_PP0_"] = "$__DFFE_PP0";
				if (patmatch(pattern, "$_DFF_PP1_")) found_match = true, direct_dict["$_DFF_PP1_"] = "$__DFFE_PP1";
				if (!found_match)
					log_cmd_error("No cell types matched pattern '%s'.\n", pattern);
				continue;
			}
			break;
		}
		extra_args(args, argidx, design);

		if (!direct_dict.empty()) {
			log("Selected cell types for direct conversion:\n");
			for (auto &it : direct_dict)
				log("  %s -> %s\n", log_id(it.first), log_id(it.second));
		}

		for (auto mod : design->selected_modules())
			if (!mod->has_processes_warn())
			{

				for (auto cell : mod->selected_cells()) {
					Dff2lutWorker worker(mod, direct_dict, cell);
				}
			}
	}
} Dff2lutPass;

PRIVATE_NAMESPACE_END
