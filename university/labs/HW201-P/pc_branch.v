// Course project reference (HW201): the program counter of F1-21 with the relative jump
// of the F1-21 mini-project. At each rising edge the first rule that applies wins:
//   reset  = 1 -> pc = 0
//   load   = 1 -> pc = jump_to            (absolute jump)
//   branch = 1 -> pc = pc + offset        (relative jump; offset is signed, two's complement)
//   inc    = 1 -> pc = pc + 1
//   otherwise     pc keeps its value
// All arithmetic is modulo 2^WIDTH (wrap-around), as in F1-17.
`timescale 1ns/1ns
module pc_branch #(
    parameter WIDTH = 8
) (
    input  wire             clk,
    input  wire             reset,
    input  wire             load,
    input  wire             branch,
    input  wire             inc,
    input  wire [WIDTH-1:0] jump_to,
    input  wire [WIDTH-1:0] offset,
    output reg  [WIDTH-1:0] pc
);
    always @(posedge clk) begin
        if (reset)
            pc <= {WIDTH{1'b0}};
        else if (load)
            pc <= jump_to;
        else if (branch)
            pc <= pc + offset;        // adding the two's-complement offset is a signed add
        else if (inc)
            pc <= pc + 1'b1;
    end
endmodule
