// A RAM of 16 words of 8 bits, built like a register file.
// Write: at a rising edge of clk, if we = 1, mem[addr] takes din.
// Read: dout always shows mem[addr] (no clock needed).
`timescale 1ns/1ns
module ram16x8 (
    input  wire       clk,
    input  wire       we,        // write enable
    input  wire [3:0] addr,      // which word: 0 to 15
    input  wire [7:0] din,
    output wire [7:0] dout
);
    reg [7:0] mem [0:15];

    always @(posedge clk)
        if (we)
            mem[addr] <= din;

    assign dout = mem[addr];
endmodule
