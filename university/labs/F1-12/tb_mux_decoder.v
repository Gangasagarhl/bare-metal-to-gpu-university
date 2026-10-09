// Testbench: every input combination of mux2, mux4 and dec2to4.
module tb_mux_decoder;
    reg d0, d1, s1;
    reg [3:0] d;
    reg [1:0] s;
    reg en;
    wire y2, y4;
    wire [3:0] yd;
    integer i;
    integer errors = 0;

    mux2    m2(.d0(d0), .d1(d1), .s(s1), .y(y2));
    mux4    m4(.d(d), .s(s), .y(y4));
    dec2to4 dc(.s(s), .en(en), .y(yd));

    initial begin
        for (i = 0; i < 8; i = i + 1) begin            // mux2: 3 inputs, 8 rows
            {s1, d1, d0} = i[2:0];
            #1;
            if (y2 !== (s1 ? d1 : d0)) errors = errors + 1;
        end
        for (i = 0; i < 64; i = i + 1) begin           // mux4: 6 inputs, 64 rows
            {s, d} = i[5:0];
            #1;
            if (y4 !== d[s]) errors = errors + 1;
        end
        $display("en s1 s0 | y3 y2 y1 y0");
        for (i = 0; i < 8; i = i + 1) begin            // decoder: 3 inputs, 8 rows
            {en, s} = i[2:0];
            #1;
            $display(" %b  %b  %b |  %b  %b  %b  %b", en, s[1], s[0], yd[3], yd[2], yd[1], yd[0]);
            if (yd !== (en ? (4'b0001 << s) : 4'b0000)) errors = errors + 1;
        end
        if (errors == 0) $display("ALL PASS (80 vectors: mux2 8, mux4 64, dec2to4 8)");
        else $display("FAIL: %0d of 80 vectors wrong", errors);
        $finish(0);
    end
endmodule
