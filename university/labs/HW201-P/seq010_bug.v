// Forensic evidence for the HW201 final: Mira's detector for the bit pattern 0, 1, 0.
// Specification: found = 1 in the cycle after the last three sampled bits were 0, 1, 0;
// overlapping occurrences count (01010 contains the pattern twice).
`timescale 1ns/1ns
module seq010_bug (
    input  wire clk,
    input  wire reset,
    input  wire in,
    output wire found
);
    localparam S0   = 2'd0,    // nothing useful seen yet
               SA   = 2'd1,    // last bit was 0
               S01  = 2'd2,    // last two bits were 0, 1
               S010 = 2'd3;    // last three bits were 0, 1, 0

    reg [1:0] state;
    reg [1:0] next;

    always @(*) begin
        case (state)
            S0:      next = in ? S0  : SA;
            SA:      next = in ? S01 : SA;
            S01:     next = in ? S0  : S010;
            default: next = S0;                 // S010: pattern reported, start again
        endcase
    end

    always @(posedge clk) begin
        if (reset)
            state <= S0;
        else
            state <= next;
    end

    assign found = (state == S010);
endmodule
