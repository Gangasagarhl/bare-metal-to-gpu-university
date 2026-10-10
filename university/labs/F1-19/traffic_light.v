// Traffic light for a crossing of a main road (north-south, NS) and a side road
// (east-west, EW). A Moore state machine: the lights depend only on the state.
// The side road gets green only when a car is waiting there (sensor car_ew = 1).
`timescale 1ns/1ns
module traffic_light (
    input  wire       clk,
    input  wire       reset,
    input  wire       car_ew,     // 1 while a car waits on the side road
    output reg  [1:0] ns,         // light of the main road
    output reg  [1:0] ew,         // light of the side road
    output reg  [2:0] state
);
    // light colours
    localparam RED = 2'd0, YELLOW = 2'd1, GREEN = 2'd2;
    // states
    localparam NS_GREEN  = 3'd0,
               NS_YELLOW = 3'd1,
               ALL_RED_1 = 3'd2,
               EW_GREEN  = 3'd3,
               EW_YELLOW = 3'd4,
               ALL_RED_2 = 3'd5;
    // how many clock cycles each state lasts (at least)
    localparam T_GREEN_NS = 4'd6, T_GREEN_EW = 4'd4, T_YELLOW = 4'd2, T_ALL_RED = 4'd1;

    reg [3:0] timer;      // clock cycles spent in the current state
    reg [2:0] next;

    // next-state logic (combinational)
    always @(*) begin
        next = state;                                  // default: stay
        case (state)
            NS_GREEN:  if (timer >= T_GREEN_NS - 1 && car_ew) next = NS_YELLOW;
            NS_YELLOW: if (timer >= T_YELLOW - 1)   next = ALL_RED_1;
            ALL_RED_1: if (timer >= T_ALL_RED - 1)  next = EW_GREEN;
            EW_GREEN:  if (timer >= T_GREEN_EW - 1) next = EW_YELLOW;
            EW_YELLOW: if (timer >= T_YELLOW - 1)   next = ALL_RED_2;
            ALL_RED_2: if (timer >= T_ALL_RED - 1)  next = NS_GREEN;
            default:   next = ALL_RED_1;               // unknown code: go to a safe state
        endcase
    end

    // state register and timer (sequential)
    always @(posedge clk) begin
        if (reset) begin
            state <= ALL_RED_2;
            timer <= 4'd0;
        end else begin
            state <= next;
            timer <= (next == state && timer != 4'd15) ? timer + 4'd1 : 4'd0;
        end
    end

    // output logic (combinational, from the state only: Moore)
    always @(*) begin
        ns = RED;
        ew = RED;
        case (state)
            NS_GREEN:  ns = GREEN;
            NS_YELLOW: ns = YELLOW;
            EW_GREEN:  ew = GREEN;
            EW_YELLOW: ew = YELLOW;
            default:   ;                               // all-red states
        endcase
    end
endmodule
