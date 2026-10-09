// Forensic evidence: the 4-to-1 multiplexer from Priya's sound board, exactly as handed in.
module mux4(input wire [3:0] d, input wire [1:0] s, output wire y);
    wire lo, hi;
    mux2 m0(.d0(d[0]), .d1(d[1]), .s(s[1]), .y(lo));
    mux2 m1(.d0(d[2]), .d1(d[3]), .s(s[1]), .y(hi));
    mux2 m2(.d0(lo),   .d1(hi),   .s(s[0]), .y(y));
endmodule
