// Testbench: print every value the ripple counter takes, including the short
// in-between values while the change ripples from bit 0 to bit 3.
`timescale 1ns/1ns
module ripple_tb;
    reg        clk = 0;
    reg        reset = 1;
    wire [3:0] count;

    ripple4 dut (.clk(clk), .reset(reset), .count(count));

    always #10 clk = ~clk;       // rising edges at 10, 30, 50, ...

    initial begin
        #2 reset = 0;
        $monitor("t=%4t  count = %2d  (%b)", $time, count, count);
        #330 $finish(0);
    end
endmodule
