`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    09:34:06 01/31/2015 
// Design Name: 
// Module Name:    part4 
// Project Name: 
// Target Devices: 
// Tool versions: 
// Description: 
//
// Dependencies: 
//
// Revision: 
// Revision 0.01 - File Created
// Additional Comments: 
//
//////////////////////////////////////////////////////////////////////////////////
module adderCLA4b(
    input [3:0] a,
    input [3:0] b,
    input SEL,
    output Cout,
    output [3:0] Sum
	 );
	 //4 bit look ahead adder
	 wire [3:0] g;
	 wire [3:0] p;
	 wire [3:0] c;
	 wire [3:0] d;
	 assign g = a & b;
	 assign p = a | b;
	 assign c[0] = g[0] | (p[0] & SEL );
 	 assign c[1] = g[1] | (p[1] & g[0]) | (p[1] & p[0] & SEL );
  	 assign c[2] = g[2] | (p[2] & g[1]) | (p[1] & p[2] & g[0]) | (p[1] & p[0] & p[2] & SEL );
  	 assign c[3] = g[3] | (p[3] & g[2]) | (p[2] & p[3] & g[1]) | (p[1] & p[2] & p[3]& g[0] )| (p[1] & p[0] & p[2] & p[3] & SEL );

   //reusing fulladder modules
	 FullAdder a0(.a(a[0]), .b(b[0]), .c(SEL), .Cout(d[0]), .Sum(Sum[0]));
	 FullAdder a1(.a(a[1]), .b(b[1]), .c(c[0]), .Cout(d[1]), .Sum(Sum[1]));
	 FullAdder a2(.a(a[2]), .b(b[2]), .c(c[1]), .Cout(d[2]), .Sum(Sum[2]));
	 FullAdder a3(.a(a[3]), .b(b[3]), .c(c[2]), .Cout(Cout), .Sum(Sum[3]));
	 

endmodule
