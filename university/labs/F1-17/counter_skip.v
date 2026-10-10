// Forensic evidence: Ana's 4-bit counter for the ticket-number display.
// It was meant to add 1 at every rising edge of clk, exactly like counter4.v.
`timescale 1ns/1ns
module counter_skip (
    input  wire       clk,
    input  wire       reset,
    input  wire       enable,
    output reg  [3:0] count,
    output wire       wrap
);
    assign wrap = enable && (count == 4'd15);

    always @(clk) begin
        if (reset)
            count <= 4'd0;
        else if (enable)
            count <= count + 4'd1;
    end
endmodule
