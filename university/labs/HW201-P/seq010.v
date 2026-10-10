// Moore state machine: found = 1 when the last three input bits were 0, 1, 0.
// Overlaps count: in 01010 the pattern is found twice (correct version).
`timescale 1ns/1ns
module seq010 (
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
            default: next = in ? S01 : SA;      // S010: its last 0 can start the next pattern
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
