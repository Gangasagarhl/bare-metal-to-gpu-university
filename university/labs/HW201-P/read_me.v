// Final exam question F14 (HW201): read this module without running it. Which regs become
// flip-flops, which become combinational logic, and which becomes a latch? Then compare with
// the synthesis tool's report in read_me_synth.out and read_me_latch.out.
`timescale 1ns/1ns
module read_me (
    input  wire       clk,
    input  wire       reset,
    input  wire       go,
    input  wire [3:0] a,
    input  wire [3:0] b,
    output reg  [4:0] total,
    output reg        big,
    output reg  [3:0] pick
);
    reg [1:0] step;

    always @(posedge clk) begin
        if (reset) begin
            step  <= 2'd0;
            total <= 5'd0;
        end else if (go) begin
            step  <= step + 2'd1;
            total <= a + b;
        end
    end

    always @(*) begin
        big = 1'b0;
        if (total > 5'd20)
            big = 1'b1;
    end

    always @(*) begin
        if (step == 2'd0)
            pick = a;
        else if (step == 2'd1)
            pick = b;
    end
endmodule
