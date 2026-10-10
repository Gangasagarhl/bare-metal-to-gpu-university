// Forensic evidence: Omar's XOR made of four NANDs, exactly as handed in.
module xor_n(input wire a, input wire b, output wire y);   // 4 NANDs
    wire t, u, v;
    nand g1(t, a, b);
    nand g2(u, a, t);
    nand g3(v, b, a);
    nand g4(y, u, v);
endmodule
