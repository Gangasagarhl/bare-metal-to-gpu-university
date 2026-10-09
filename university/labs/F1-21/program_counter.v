// Program counter for a small CPU (the course project builds on it in HW202).
// At each rising edge of clk, the first rule that applies wins:
//   reset = 1 -> pc = 0
//   load  = 1 -> pc = jump_to      (a jump)
//   inc   = 1 -> pc = pc + 1       (the next instruction)
//   otherwise    pc keeps its value
`timescale 1ns/1ns
module program_counter #(
    parameter WIDTH = 8                    // number of address bits
) (
    input  wire             clk,
    input  wire             reset,
    input  wire             load,
    input  wire             inc,
    input  wire [WIDTH-1:0] jump_to,
    output reg  [WIDTH-1:0] pc
);
    always @(posedge clk) begin
        if (reset)
            pc <= {WIDTH{1'b0}};
        else if (load)
            pc <= jump_to;
        else if (inc)
            pc <= pc + 1'b1;
    end
endmodule
