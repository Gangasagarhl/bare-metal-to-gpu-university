// Run a decade counter for 25 edges and check the digits 0..9 repeat.
// Compile with -DDUT=decade or -DDUT=decade_big.
`timescale 1ns/1ns
module decade_tb;
    reg        clk = 0;
    reg        reset = 1;
    wire [3:0] digit;
    integer    k;
    integer    errors = 0;

    `DUT dut (.clk(clk), .reset(reset), .digit(digit));

    always #5 clk = ~clk;

    initial begin
        @(posedge clk); #1 reset = 0;
        $write("digits:");
        for (k = 1; k <= 25; k = k + 1) begin
            @(posedge clk); #1;
            $write(" %0d", digit);
            if (digit !== k % 10) errors = errors + 1;
        end
        $write("\n");
        if (errors == 0)
            $display("RESULT: PASS (0 to 9, then again)");
        else
            $display("RESULT: FAIL (%0d wrong digits)", errors);
        $finish(0);
    end
endmodule
