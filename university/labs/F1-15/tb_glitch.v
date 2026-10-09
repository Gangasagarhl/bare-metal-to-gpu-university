// Testbench: both data inputs are 1, so the right answer is 1 whatever s is.
// Watch both multiplexers while s changes from 1 to 0.
module tb_glitch;
    reg d0, d1, s;
    wire y_plain, y_covered;

    mux2_d       m1(.d0(d0), .d1(d1), .s(s), .y(y_plain));
    mux2_covered m2(.d0(d0), .d1(d1), .s(s), .y(y_covered));

    initial begin
        d0 = 1; d1 = 1; s = 1;
        #10;
        $display("time | s | plain: ns t0 t1 y | covered y");
        $monitor("%4t | %b |         %b  %b  %b %b |     %b", $time, s,
                 m1.ns, m1.t0, m1.t1, y_plain, y_covered);
        s = 0;                                 // the change, at time 10
        #10;
        $finish(0);
    end
endmodule
