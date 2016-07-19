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

#include "kernel/register.h"
#include "kernel/celltypes.h"
#include "kernel/rtlil.h"
#include "kernel/log.h"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct SynthTritonCorePass : public ScriptPass
{
	SynthTritonCorePass() : ScriptPass("synth_tritoncore", "synthesis for TRITONCORE FPGAs") { }

	virtual void help() YS_OVERRIDE
	{
		//   |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
		log("\n");
		log("    synth_tritoncore [options]\n");
		log("\n");
		log("This command runs synthesis for TRITONCORE FPGAs. This work is experimental.\n");
		log("\n");
		log("    -top <module>\n");
		log("        use the specified module as top module (default='top')\n");
		log("\n");
		log("    -blif <file>\n");
		log("        write the design to the specified BLIF file. writing of an output file\n");
		log("        is omitted if this parameter is not specified.\n");
		log("\n");
		log("    -edif <file>\n");
		log("        write the design to the specified edif file. writing of an output file\n");
		log("        is omitted if this parameter is not specified.\n");
		log("\n");
		log("    -run <from_label>:<to_label>\n");
		log("        only run the commands between the labels (see below). an empty\n");
		log("        from label is synonymous to 'begin', and empty to label is\n");
		log("        synonymous to the end of the command list.\n");
		log("\n");
		log("    -noflatten\n");
		log("        do not flatten design before synthesis\n");
		log("\n");
		log("    -retime\n");
		log("        run 'abc' with -dff option\n");
		log("\n");
		log("    -abc2\n");
		log("        run two passes of 'abc' for slightly improved logic density\n");
		log("\n");
		log("The following commands are executed by this synthesis command:\n");
		help_script();
		log("\n");
	}

	string top_opt, blif_file, edif_file;
	bool nocarry, nobram, flatten, retime, abc2;

	virtual void clear_flags() YS_OVERRIDE
	{
		top_opt = "-auto-top";
		blif_file = "";
		edif_file = "";
		nocarry = false;
		nobram = false;
		flatten = true;
		retime = false;
		abc2 = false;
	}

	virtual void execute(std::vector<std::string> args, RTLIL::Design *design) YS_OVERRIDE
	{
		string run_from, run_to;
		clear_flags();

		size_t argidx;
		for (argidx = 1; argidx < args.size(); argidx++)
		{
			if (args[argidx] == "-top" && argidx+1 < args.size()) {
				top_opt = "-top " + args[++argidx];
				continue;
			}
			if (args[argidx] == "-blif" && argidx+1 < args.size()) {
				blif_file = args[++argidx];
				continue;
			}
			if (args[argidx] == "-edif" && argidx+1 < args.size()) {
				edif_file = args[++argidx];
				continue;
			}
			if (args[argidx] == "-run" && argidx+1 < args.size()) {
				size_t pos = args[argidx+1].find(':');
				if (pos == std::string::npos)
					break;
				run_from = args[++argidx].substr(0, pos);
				run_to = args[argidx].substr(pos+1);
				continue;
			}
			if (args[argidx] == "-flatten") {
				flatten = true;
				continue;
			}
			if (args[argidx] == "-noflatten") {
				flatten = false;
				continue;
			}
			if (args[argidx] == "-retime") {
				retime = true;
				continue;
			}


			if (args[argidx] == "-abc2") {
				abc2 = true;
				continue;
			}
			break;
		}
		extra_args(args, argidx, design);

		if (!design->full_selection())
			log_cmd_error("This comannd only operates on fully selected designs!\n");

		log_header(design, "Executing SYNTH_TRITONCORE pass.\n");
		log_push();

		run_script(design, run_from, run_to);

		log_pop();
	}

	virtual void script() YS_OVERRIDE
	{
		if (check_label("begin"))
		{
			run("read_verilog -lib +/tritoncore/cells_sim.v");
			run(stringf("hierarchy -check %s", help_mode ? "-top <top>" : top_opt.c_str()));
		}

		if (flatten && check_label("flatten", "(unless -noflatten)"))
		{
			run("proc");
			run("flatten");

		}

		if (check_label("coarse"))
		{
			run("synth -run coarse");
		}

		if (check_label("fine"))
		{
			run("opt -fast -mux_undef -undriven -fine");
			run("memory_map");
			run("opt -undriven -fine");
            run("techmap");
			if (retime || help_mode)
				run("abc -dff", "(only if -retime)");
			run("tritoncore_opt");
		}

		if (check_label("map_ffs"))
		{
			run("dffsr2dff");
			run("dff2lut -direct-match $_DFF_*");
			run("techmap -map +/tritoncore/cells_map.v");
			run("opt_expr -mux_undef");
			run("simplemap");
			run("tritoncore_ffinit");
			run("tritoncore_ffssr");
			run("tritoncore_opt -full");
		}

		if (check_label("map_luts"))
		{
			if (abc2 || help_mode) {
				run("abc", "      (only if -abc2)");
				run("tritoncore_opt", "(only if -abc2)");
			}
			run("abc -lut 4");
			run("clean");
		}

		if (check_label("map_cells"))
		{
			run("techmap -map +/tritoncore/cells_map.v");
			run("clean");
		}

		if (check_label("check"))
		{
			run("hierarchy -check");
			run("stat");
			run("check -noinit");
		}

		if (check_label("blif"))
		{
			if (!blif_file.empty() || help_mode)
				run(stringf("write_blif -gates -attr -param %s", help_mode ? "<file-name>" : blif_file.c_str()));
		}

		if (check_label("edif"))
		{
			if (!edif_file.empty() || help_mode)
				run(stringf("write_edif %s", help_mode ? "<file-name>" : edif_file.c_str()));
		}
	}
} SynthTritonCorePass;

PRIVATE_NAMESPACE_END
