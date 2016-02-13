module full

  (sel, i1, i2, i3, i4, o1);

input [1:0] sel;

input [1:0] i1, i2, i3, i4;

output [1:0] o1;

  reg [1:0] o1;

always @(sel or i1 or i2 or i3 or i4)

  begin

    case (sel)

      2'b00: o1 = i1;

      2'b01: o1 = i2;

      2'b10: o1 = i3;

      2'b11: o1 = i4;

    endcase

  end

endmodule 
