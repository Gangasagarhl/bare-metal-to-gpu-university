// HW201 practical exam (P): reference solution (Lab Engineer). Six states; a 3-bit timer
// counts the cycles already spent in the current phase; "resume" remembers which phase
// (or which next phase) to return to after a pause. Moore outputs.
`timescale 1ns/1ns
module dishwasher (
    input  wire       clk,
    input  wire       reset,
    input  wire       start,
    input  wire       door_open,
    output reg        wash,
    output reg        rinse,
    output reg        dry,
    output reg        done,
    output reg  [2:0] state
);
    localparam IDLE  = 3'd0,
               WASH  = 3'd1,
               RINSE = 3'd2,
               DRY   = 3'd3,
               DONE  = 3'd4,
               PAUSE = 3'd5;
    localparam T_WASH = 3'd6, T_RINSE = 3'd4, T_DRY = 3'd3;   // exercise values, in cycles

    reg [2:0] next;
    reg [2:0] timer;        // cycles already spent in the current phase (0 .. 5)
    reg [2:0] resume;       // the state to return to when the door closes again

    // length of the current phase, and the phase that follows it
    wire       in_phase   = (state == WASH) || (state == RINSE) || (state == DRY);
    wire [2:0] phase_len  = (state == WASH) ? T_WASH : (state == RINSE) ? T_RINSE : T_DRY;
    wire       phase_done = in_phase && (timer == phase_len - 3'd1);
    wire [2:0] after      = (state == WASH) ? RINSE : (state == RINSE) ? DRY : DONE;

    // next-state logic
    always @(*) begin
        next = state;
        case (state)
            IDLE:  if (start && !door_open) next = WASH;
            WASH, RINSE, DRY:
                   if (door_open)       next = PAUSE;
                   else if (phase_done) next = after;
            DONE:  if (door_open) next = IDLE;
            PAUSE: if (!door_open) next = resume;
            default: next = IDLE;          // illegal codes 6 and 7
        endcase
    end

    // state register, timer and resume register
    always @(posedge clk) begin
        if (reset) begin
            state  <= IDLE;
            timer  <= 3'd0;
            resume <= WASH;
        end else begin
            state <= next;
            if (in_phase) begin
                if (phase_done) begin        // this cycle finished the phase
                    timer  <= 3'd0;
                    resume <= after;
                end else begin               // one more cycle spent in this phase
                    timer  <= timer + 3'd1;
                    resume <= state;
                end
            end else if (state != PAUSE) begin   // IDLE, DONE: nothing to resume
                timer  <= 3'd0;
                resume <= WASH;
            end                                  // PAUSE: hold timer and resume
        end
    end

    // output logic (Moore)
    always @(*) begin
        wash  = (state == WASH);
        rinse = (state == RINSE);
        dry   = (state == DRY);
        done  = (state == DONE);
    end
endmodule
