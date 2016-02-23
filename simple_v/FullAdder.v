`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    19:45:17 01/30/2015 
// Design Name: 
// Module Name:    FullAdder 
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
module FullAdder(
    input a,
    input b,
    input c,
    output Cout,
    output Sum
    );
    wire c1, c2;
	 wire s1;
	 //reusing half adder modules
	 HalfAdder h1( .a(a), .b(b), .c(c1), .s(s1));
	 HalfAdder h2( .a(c), .b(s1), .c(c2), .s(Sum));
	 or( Cout, c2 , c1);

endmodule
