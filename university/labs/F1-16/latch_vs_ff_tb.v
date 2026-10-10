// Testbench: the same d drives a D latch (enabled by clk), an edge-triggered
// D flip-flop, and a master-slave flip-flop made of two latches.
`timescale 1ns/1ns
module latch_vs_ff_tb;
    reg  clk = 0;
    reg  d = 0;
    wire q_latch;
    wire q_ff;
    wire q_ms;
    integer errors = 0;

    d_latch     u1 (.en(clk), .d(d), .q(q_latch));
    d_flipflop  u2 (.clk(clk), .d(d), .q(q_ff));
    ms_flipflop u3 (.clk(clk), .d(d), .q(q_ms));

    always #10 clk = ~clk;          // one clock period = 20 time units

    // d changes at awkward moments: some while clk is high, some while it is low
    initial begin
        $dumpfile("latch_vs_ff.vcd");
        $dumpvars(0, latch_vs_ff_tb);
        #5  d = 1;      // t = 5:  clk is low
        #10 d = 0;      // t = 15: clk is high, the latch passes this change through
        #3  d = 1;      // t = 18
        #9  d = 0;      // t = 27
        #8  d = 1;      // t = 35
        #20 d = 0;      // t = 55: clk is high
        #3  d = 1;      // t = 58
        #4  d = 0;      // t = 62: clk is low
        #13 d = 1;      // t = 75
        #20 d = 0;      // t = 95
        #22 d = 1;      // t = 117
        #3  d = 0;      // t = 120
        #40 $finish(0);
    end

    // after every rising edge, both flip-flops must hold the value d had at that edge
    reg d_at_edge;
    always @(posedge clk) begin
        d_at_edge = d;
        #2;
        $display("t=%0t rising edge: d was %b -> q_ff = %b, q_ms = %b, q_latch = %b",
                 $time - 2, d_at_edge, q_ff, q_ms, q_latch);
        if (q_ff !== d_at_edge || q_ms !== d_at_edge)
            errors = errors + 1;
    end

    initial begin
        #158;
        if (errors == 0)
            $display("RESULT: PASS (both flip-flops captured d at every rising edge)");
        else
            $display("RESULT: FAIL (%0d mismatches)", errors);
    end
endmodule
