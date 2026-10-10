// Forensic evidence: Priya's 3-stage delay line. Specification: three flip-flop stages;
// the value at "in" at edge n must appear at "out" just after edge n+2.
`timescale 1ns/1ns
module delay_line_bug (
    input  wire       clk,
    input  wire [3:0] in,
    output reg  [3:0] out
);
    reg [3:0] stage1;
    reg [3:0] stage2;

    always @(posedge clk) begin
        stage1 = in;
        stage2 = stage1;
        out    = stage2;
    end
endmodule
