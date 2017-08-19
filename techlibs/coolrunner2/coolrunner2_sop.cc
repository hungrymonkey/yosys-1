/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2017 Robert Ou <rqou@robertou.com>
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

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct Coolrunner2SopPass : public Pass {
	Coolrunner2SopPass() : Pass("coolrunner2_sop", "break $sop cells into ANDTERM/ORTERM cells") { }
	virtual void help()
	{
		log("\n");
		log("    coolrunner2_sop [options] [selection]\n");
		log("\n");
		log("Break $sop cells into ANDTERM/ORTERM cells.\n");
		log("\n");
	}
	virtual void execute(std::vector<std::string> args, RTLIL::Design *design)
	{
		log_header(design, "Executing COOLRUNNER2_SOP pass (break $sop cells into ANDTERM/ORTERM cells).\n");
		extra_args(args, 1, design);

		// Find all the $_NOT_ cells
		dict<SigBit, tuple<SigBit, Cell*>> not_cells;
		for (auto module : design->selected_modules())
		{
			SigMap sigmap(module);
			for (auto cell : module->selected_cells())
			{
				if (cell->type == "$_NOT_")
				{
					auto not_input = cell->getPort("\\A")[0];
					auto not_output = cell->getPort("\\Y")[0];
					not_cells[not_input] = tuple<SigBit, Cell*>(not_output, cell);
				}
			}
		}

		pool<tuple<Module*, Cell*>> cells_to_remove;
		for (auto module : design->selected_modules())
		{
			SigMap sigmap(module);
			for (auto cell : module->selected_cells())
			{
				if (cell->type == "$sop")
				{
					// Read the inputs/outputs/parameters of the $sop cell
					auto sop_inputs = sigmap(cell->getPort("\\A"));
					auto sop_output = sigmap(cell->getPort("\\Y"))[0];
					auto sop_depth = cell->getParam("\\DEPTH").as_int();
					auto sop_width = cell->getParam("\\WIDTH").as_int();
					auto sop_table = cell->getParam("\\TABLE");

					// Check for a $_NOT_ at the output
					bool has_invert = false;
					if (not_cells.count(sop_output))
					{
						auto not_cell = not_cells.at(sop_output);

						has_invert = true;
						sop_output = std::get<0>(not_cell);

						// remove the $_NOT_ cell because it gets folded into the xor
						cells_to_remove.insert(tuple<Module*, Cell*>(module, std::get<1>(not_cell)));
					}

					// Construct AND cells
					pool<SigBit> intermed_wires;
					for (int i = 0; i < sop_depth; i++) {
						// Wire for the output
						auto and_out = module->addWire(NEW_ID);
						intermed_wires.insert(and_out);

						// Signals for the inputs
						pool<SigBit> and_in_true;
						pool<SigBit> and_in_comp;
						for (int j = 0; j < sop_width; j++)
						{
							if (sop_table[2 * (i * sop_width + j) + 0])
							{
								and_in_comp.insert(sop_inputs[j]);
							}
							if (sop_table[2 * (i * sop_width + j) + 1])
							{
								and_in_true.insert(sop_inputs[j]);
							}
						}

						// Construct the cell
						auto and_cell = module->addCell(NEW_ID, "\\ANDTERM");
						and_cell->setParam("\\TRUE_INP", GetSize(and_in_true));
						and_cell->setParam("\\COMP_INP", GetSize(and_in_comp));
						and_cell->setPort("\\OUT", and_out);
						and_cell->setPort("\\IN", and_in_true);
						and_cell->setPort("\\IN_B", and_in_comp);
					}

					if (sop_depth == 1)
					{
						// If there is only one term, don't construct an OR cell. Directly construct the XOR gate
						auto xor_cell = module->addCell(NEW_ID, "\\MACROCELL_XOR");
						xor_cell->setParam("\\INVERT_OUT", has_invert);
						xor_cell->setPort("\\IN_PTC", *intermed_wires.begin());
						xor_cell->setPort("\\OUT", sop_output);
					}
					else
					{
						// Wire from OR to XOR
						auto or_to_xor_wire = module->addWire(NEW_ID);

						// Construct the OR cell
						auto or_cell = module->addCell(NEW_ID, "\\ORTERM");
						or_cell->setParam("\\WIDTH", sop_depth);
						or_cell->setPort("\\IN", intermed_wires);
						or_cell->setPort("\\OUT", or_to_xor_wire);

						// Construct the XOR cell
						auto xor_cell = module->addCell(NEW_ID, "\\MACROCELL_XOR");
						xor_cell->setParam("\\INVERT_OUT", has_invert);
						xor_cell->setPort("\\IN_ORTERM", or_to_xor_wire);
						xor_cell->setPort("\\OUT", sop_output);
					}

					// Finally, remove the $sop cell
					cells_to_remove.insert(tuple<Module*, Cell*>(module, cell));
				}
			}
		}

		// Actually do the removal now that we aren't iterating
		for (auto mod_and_cell : cells_to_remove)
		{
			std::get<0>(mod_and_cell)->remove(std::get<1>(mod_and_cell));
		}
	}
} Coolrunner2SopPass;

PRIVATE_NAMESPACE_END
