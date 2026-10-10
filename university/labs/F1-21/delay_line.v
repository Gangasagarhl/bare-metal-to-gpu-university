// The same delay line written with non-blocking assignments (<=): every stage takes the
// value its neighbour had BEFORE the edge, so data moves one stage per edge.
`timescale 1ns/1ns
module delay_line (
    input  wire       clk,
    input  wire [3:0] in,
    output reg  [3:0] out
);
    reg [3:0] stage1;
    reg [3:0] stage2;

    always @(posedge clk) begin
        stage1 <= in;
        stage2 <= stage1;
        out    <= stage2;
    end
endmodule
