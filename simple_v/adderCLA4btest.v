`timescale 1ns / 1ps

////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer:
//
// Create Date:   12:27:40 01/31/2015
// Design Name:   part4
// Module Name:   H:/140l/hw2/part4test.v
// Project Name:  Lab-2
// Target Device:  
// Tool versions:  
// Description: 
//
// Verilog Test Fixture created by ISE for module: part4
//
// Dependencies:
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
////////////////////////////////////////////////////////////////////////////////

module adderCLA4btest;

	// Inputs
	reg [3:0] a;
	reg [3:0] b;
	reg SEL;

	// Outputs
	wire Cout;
	wire [3:0] Sum;

	// Instantiate the Unit Under Test (UUT)
	adderCLA4b uut (
		.a(a), 
		.b(b), 
		.SEL(SEL), 
		.Cout(Cout), 
		.Sum(Sum)
	);

	initial begin
		$monitor("%d  %d | %d | %d %d", a, b, SEL, Cout, Sum);

		// Initialize Inputs
		a = 0;
		b = 0;
		SEL = 0;

		// Wait 100 ns for global reset to finish
		#100;
        
		// Add stimulus here
		// Initialize Inputs
		a = 1;
		b = 0;
		SEL = 0;

		// Wait 100 ns for global reset to finish
		#100;
        
		// Add stimulus here
		// Initialize Inputs
		a = 1;
		b = 1;
		SEL = 1;
		// Wait 100 ns for global reset to finish
		#100;
				// Initialize Inputs
		a = 4;
		b = 6;
		SEL = 0;

		// Wait 100 ns for global reset to finish
		#100;
				// Initialize Inputs
		a = 5;
		b = 0;
		SEL = 0;
		#100;
		a = 3;
		b = 11;
		SEL = 0;
		// Wait 100 ns for global reset to finish
		#100;
				// Initialize Inputs
		a = 15;
		b = 15;
		SEL = 1;

		// Wait 100 ns for global reset to finish
		#100;
		a = 7;
		b = 8;
		SEL = 0;

		// Wait 100 ns for global reset to finish
		#100;
						// Initialize Inputs
		a = 8;
		b = 8;
		SEL = 0;

		// Wait 100 ns for global reset to finish
		#100;
	end
      
endmodule

