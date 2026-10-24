// Model answer of final question F10 (HW201): the door chime, a Moore machine.
// chime = 1 for exactly one cycle each time the door goes from closed to open; it does not
// sound again while the door stays open. After reset the door is taken to be closed.
`timescale 1ns/1ns
module chime (
    input  wire clk,
    input  wire reset,
    input  wire door,       // 1 = open, sampled at each rising edge
    output reg  chime
);
    localparam CLOSED    = 2'd0,   // the door was closed at the last edge
               JUST_OPEN = 2'd1,   // the door has just opened: sound the chime
               STAY_OPEN = 2'd2;   // the door is still open: already chimed

    reg [1:0] state;
    reg [1:0] next;

    // next-state logic: an arrow for every input in every state, a default for code 3
    always @(*) begin
        next = CLOSED;
        case (state)
            CLOSED:    next = door ? JUST_OPEN : CLOSED;
            JUST_OPEN: next = door ? STAY_OPEN : CLOSED;
            STAY_OPEN: next = door ? STAY_OPEN : CLOSED;
            default:   next = CLOSED;          // illegal code 3
        endcase
    end

    // state register with synchronous reset
    always @(posedge clk) begin
        if (reset)
            state <= CLOSED;
        else
            state <= next;
    end

    // output logic (Moore): the chime depends on the state only
    always @(*) begin
        chime = 1'b0;
        if (state == JUST_OPEN)
            chime = 1'b1;
    end
endmodule
