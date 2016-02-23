`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: Ted Chang #11636556
// 
// Create Date:    19:41:41 01/30/2015 
// Design Name: 
// Module Name:    HalfAdder 
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
module HalfAdder(
    input a,
    input b,
    output c,
    output s
    );
	assign c = a & b;
	assign s = a ^ b;

endmodule
