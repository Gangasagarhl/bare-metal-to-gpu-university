// Evidence for midterm question M11: a 4-to-1 multiplexer tree, as handed in.
module mux4(input wire [3:0] d, input wire [1:0] s, output wire y);
    wire lo, hi;
    mux2 m0(.d0(d[1]), .d1(d[0]), .s(s[0]), .y(lo));
    mux2 m1(.d0(d[3]), .d1(d[2]), .s(s[0]), .y(hi));
    mux2 m2(.d0(lo),   .d1(hi),   .s(s[1]), .y(y));
endmodule
