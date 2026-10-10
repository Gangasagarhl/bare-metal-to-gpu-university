// Course project reference (HW201): the memory unit for the HW202 CPU. 256 words of 8 bits,
// synchronous write with write enable, asynchronous read (the F1-20 mini-project).
`timescale 1ns/1ns
module ram256x8 (
    input  wire       clk,
    input  wire       we,
    input  wire [7:0] addr,
    input  wire [7:0] din,
    output wire [7:0] dout
);
    reg [7:0] mem [0:255];

    always @(posedge clk)
        if (we)
            mem[addr] <= din;

    assign dout = mem[addr];
endmodule
