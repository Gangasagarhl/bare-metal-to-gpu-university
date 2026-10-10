// HW201 practical exam (P): starting file. Complete this module so that dishwasher_tb.v
// prints RESULT: PASS. Keep the file name, the module name and the ports; everything
// else is yours. Course commands (from the folder that holds both files):
//   sim:   iverilog -g2005 -Wall -o dishwasher.vvp dishwasher_tb.v dishwasher.v && vvp -n dishwasher.vvp
//   lint:  verilator --lint-only -Wall dishwasher.v
//   synth: yosys -q -p 'read_verilog dishwasher.v; synth -top dishwasher; tee -q -o /dev/stdout stat'
//
// Word description (the paper has the full text):
//   Inputs are sampled at the rising edge of clk. After reset: IDLE, all lamps off.
//   IDLE: start = 1 with door_open = 0 begins a programme: WASH for exactly 6 cycles
//   (wash = 1), then RINSE for exactly 4 cycles (rinse = 1), then DRY for exactly 3 cycles
//   (dry = 1), then DONE (done = 1). DONE lasts until door_open = 1 is sampled; then IDLE.
//   start is ignored in DONE and while the door is open.
//   door_open = 1 during WASH, RINSE or DRY pauses the programme: all lamps off while the
//   door is open; when door_open = 0 is sampled, the interrupted phase continues, and the
//   cycles already spent in it still count (6 + 4 + 3 lamp cycles in every programme).
//   At most one lamp is on at any time (Moore outputs: lamps depend on the state only).
//   state is for your own debugging: the testbench prints it but does not check it.
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
    // TODO 1: name your states (localparam) and declare what you need next to the state
    //         register: the next-state value, a cycle timer, where to resume after a pause.
    localparam IDLE = 3'd0;

    // TODO 2: next-state logic: always @(*) with a default assignment, one case per state.

    // TODO 3: state register with reset (keep the timer and the resume register here too).
    //         As handed out, the register only resets; it never leaves IDLE.
    always @(posedge clk) begin
        if (reset)
            state <= IDLE;
    end

    // TODO 4: output logic: all lamps 0 by default, then the lamp of the current state.
    always @(*) begin
        wash  = 1'b0;
        rinse = 1'b0;
        dry   = 1'b0;
        done  = 1'b0;
        case (state)
            default: ;
        endcase
    end
endmodule
