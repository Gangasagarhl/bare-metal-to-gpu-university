// Testbench: set, hold, reset, hold, then the forbidden input S = R = 1 and its release.
`timescale 1ns/1ns
module sr_latch_tb;
    reg  s = 0;
    reg  r = 0;
    wire q;
    wire qn;

    sr_latch dut (.s(s), .r(r), .q(q), .qn(qn));

    initial begin
        $display("time  S R | Q Qn   (x = unknown)");
        $monitor("%4t  %b %b | %b %b", $time, s, r, q, qn);
        #5 s = 1;           // set
        #5 s = 0;           // hold: the latch remembers 1
        #5 r = 1;           // reset
        #5 r = 0;           // hold: the latch remembers 0
        #5 s = 1; r = 1;    // forbidden: both inputs active
        #5 s = 0; r = 0;    // release both at the same instant
        #10 $finish(0);
    end
endmodule
