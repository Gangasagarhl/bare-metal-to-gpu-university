// Evidence for midterm question M8: a four-NAND XOR with one wrong connection.
module xor_n(input wire a, input wire b, output wire y);   // 4 NANDs
    wire t, u, v;
    nand g1(t, a, b);
    nand g2(u, a, b);       // should be nand g2(u, a, t);
    nand g3(v, b, t);
    nand g4(y, u, v);
endmodule
