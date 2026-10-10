// Testbench: one-hot probe of a 4-to-1 multiplexer. Only input d[k] is 1;
// for each select value, which k comes out?
module tb_mux4_probe;
    reg [3:0] d;
    reg [1:0] s;
    wire y;
    integer k, j;
    integer errors = 0;

    mux4 dut(.d(d), .s(s), .y(y));

    initial begin
        $display("select s1 s0 | output is 1 when the only 1 is on input ...");
        for (j = 0; j < 4; j = j + 1) begin
            s = j[1:0];
            $write("         %b  %b |", s[1], s[0]);
            for (k = 0; k < 4; k = k + 1) begin
                d = 4'b0001 << k;
                #1;
                if (y === 1'b1) $write(" d%0d", k);
                if (y !== d[s]) errors = errors + 1;
            end
            $write("   (expected d%0d)\n", j);
        end
        if (errors == 0) $display("ALL PASS (16 probes)");
        else $display("FAIL: %0d of 16 probes wrong", errors);
        $finish(0);
    end
endmodule
