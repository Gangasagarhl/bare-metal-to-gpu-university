// Measure the period of div2 and div4 and the spacing of tick pulses, in time units.
`timescale 1ns/1ns
module divider_tb;
    reg     clk = 0;
    reg     reset = 1;
    wire    div2;
    wire    div4;
    wire    tick;
    integer last2 = -1;
    integer last4 = -1;
    integer lastt = -1;
    integer p2 = 0;
    integer p4 = 0;
    integer pt = 0;

    divider dut (.clk(clk), .reset(reset), .div2(div2), .div4(div4), .tick(tick));

    always #5 clk = ~clk;        // clock period 10 time units

    initial begin
        $dumpfile("divider.vcd");
        $dumpvars(0, divider_tb);
        #12 reset = 0;
        #200;
        $display("clk  period = 10 time units");
        $display("div2 period = %0d time units", p2);
        $display("div4 period = %0d time units", p4);
        $display("tick pulses every %0d time units", pt);
        if (p2 == 20 && p4 == 40 && pt == 50)
            $display("RESULT: PASS (periods 2, 4 and 5 times the clock period)");
        else
            $display("RESULT: FAIL");
        $finish(0);
    end

    // measure from one rising edge of each signal to the next
    always @(posedge div2) begin if (last2 >= 0) p2 = $time - last2; last2 = $time; end
    always @(posedge div4) begin if (last4 >= 0) p4 = $time - last4; last4 = $time; end
    always @(posedge tick) begin if (lastt >= 0) pt = $time - lastt; lastt = $time; end
endmodule
