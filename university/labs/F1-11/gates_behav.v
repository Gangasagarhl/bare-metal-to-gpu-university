// The same functions written as plain expressions, for the synthesis tool to map onto NANDs.
module xor2(input wire a, input wire b, output wire y);
    assign y = a ^ b;
endmodule

module or2(input wire a, input wire b, output wire y);
    assign y = a | b;
endmodule
