// Simulate board_top with DIV = 4: the digit must step once every 4 clock cycles
// and wrap from 9 to 0.
`timescale 1ns/1ns
module board_tb;
    reg        clk = 0;
    reg        btn_reset = 1;
    wire [3:0] led;
    integer    cycle;
    integer    errors = 0;
    integer    expected;

    board_top #(.DIV(4)) dut (.clk(clk), .btn_reset(btn_reset), .led(led));

    always #5 clk = ~clk;

    initial begin
        @(posedge clk); #1 btn_reset = 0;
        $write("led after each 4th cycle:");
        for (cycle = 1; cycle <= 48; cycle = cycle + 1) begin
            @(posedge clk); #1;
            expected = (cycle / 4) % 10;   // one step at cycles 4, 8, 12, ...
            if (led !== expected) errors = errors + 1;
            if (cycle % 4 == 0) $write(" %0d", led);
        end
        $write("\n");
        if (errors == 0)
            $display("RESULT: PASS (48 cycles, one step every 4 cycles, 9 wraps to 0)");
        else
            $display("RESULT: FAIL (%0d wrong cycles)", errors);
        $finish(0);
    end
endmodule
