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
#include "passes/techmap/simplemap.h"
#include <stdlib.h>
#include <stdio.h>

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

static void run_tritoncore_opts(Module *module)
{
	pool<SigBit> optimized_co;
	vector<Cell*> sb_lut_cells;
	SigMap sigmap(module);

	for (auto cell : module->selected_cells())
	{
		if (cell->type == "\\SB_LUT4")
		{
			sb_lut_cells.push_back(cell);
			continue;
		}

	}

	for (auto cell : sb_lut_cells)
	{
		SigSpec inbits;

		inbits.append(cell->getPort("\\I0"));
		inbits.append(cell->getPort("\\I1"));
		inbits.append(cell->getPort("\\I2"));
		inbits.append(cell->getPort("\\I3"));
		sigmap.apply(inbits);

		if (optimized_co.count(inbits[0])) goto remap_lut;
		if (optimized_co.count(inbits[1])) goto remap_lut;
		if (optimized_co.count(inbits[2])) goto remap_lut;
		if (optimized_co.count(inbits[3])) goto remap_lut;

		if (!sigmap(inbits).is_fully_const())
			continue;

	remap_lut:
		module->design->scratchpad_set_bool("opt.did_something", true);
		log("Mapping SB_LUT4 cell %s.%s back to logic.\n", log_id(module), log_id(cell));

		cell->type ="$lut";
		cell->setParam("\\WIDTH", 4);
		cell->setParam("\\LUT", cell->getParam("\\LUT_INIT"));
		cell->unsetParam("\\LUT_INIT");

		cell->setPort("\\A", SigSpec({cell->getPort("\\I0"), cell->getPort("\\I1"), cell->getPort("\\I2"), cell->getPort("\\I3")}));
		cell->setPort("\\Y", cell->getPort("\\O"));
		cell->unsetPort("\\I0");
		cell->unsetPort("\\I1");
		cell->unsetPort("\\I2");
		cell->unsetPort("\\I3");
		cell->unsetPort("\\O");

		cell->check();
		simplemap_lut(module, cell);
		module->remove(cell);
	}
}

struct Ice40OptPass : public Pass {
	Ice40OptPass() : Pass("tritoncore_opt", "tritonCore: perform simple optimizations") { }
	virtual void help()
	{
		//   |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
		log("\n");
		log("    tritoncore_opt [options] [selection]\n");
		log("\n");
		log("This command executes the following script:\n");
		log("\n");
		log("    do\n");
		log("        <tritoncore specific optimizations>\n");
		log("        opt_const -mux_undef -undriven [-full]\n");
		log("        opt_share\n");
		log("        opt_rmdff\n");
		log("        opt_clean\n");
		log("    while <changed design>\n");
		log("\n");
	}
	virtual void execute(std::vector<std::string> args, RTLIL::Design *design)
	{
		string opt_const_args = "-mux_undef -undriven";
		log_header("Executing TRITONCORE_OPT pass (performing simple optimizations).\n");
		log_push();

		size_t argidx;
		for (argidx = 1; argidx < args.size(); argidx++) {
			if (args[argidx] == "-full") {
				opt_const_args += " -full";
				continue;
			}
			break;
		}
		extra_args(args, argidx, design);

		while (1)
		{
			design->scratchpad_unset("opt.did_something");

			log_header("Running TRITONCORE specific optimizations.\n");
			for (auto module : design->selected_modules())
				run_tritoncore_opts(module);

			Pass::call(design, "opt_const " + opt_const_args);
			Pass::call(design, "opt_share");
			Pass::call(design, "opt_rmdff");
			Pass::call(design, "opt_clean");

			if (design->scratchpad_get_bool("opt.did_something") == false)
				break;

			log_header("Rerunning OPT passes. (Removed registers in this run.)\n");
		}

		design->optimize();
		design->sort();
		design->check();

		log_header("Finished OPT passes. (There is nothing left to do.)\n");
		log_pop();
	}
} Ice40OptPass;

PRIVATE_NAMESPACE_END
