// Moore state machine: found = 1 when the last three input bits were 1, 0, 1.
// Overlaps count: in 10101 the pattern is found twice.
`timescale 1ns/1ns
module seq101 (
    input  wire clk,
    input  wire reset,
    input  wire in,
    output wire found
);
    localparam S0 = 2'd0,      // nothing useful seen yet
               S1 = 2'd1,      // last bit was 1
               S10 = 2'd2,     // last two bits were 1, 0
               S101 = 2'd3;    // last three bits were 1, 0, 1

    reg [1:0] state;
    reg [1:0] next;

    always @(*) begin
        case (state)
            S0:      next = in ? S1   : S0;
            S1:      next = in ? S1   : S10;
            S10:     next = in ? S101 : S0;
            default: next = in ? S1   : S10;     // S101
        endcase
    end

    always @(posedge clk) begin
        if (reset)
            state <= S0;
        else
            state <= next;
    end

    assign found = (state == S101);
endmodule
