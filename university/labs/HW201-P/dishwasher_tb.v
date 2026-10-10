// HW201 practical exam (P): the testbench handed to candidates together with
// start/dishwasher.v. It drives the controller through three programmes (a clean run,
// a run with the door opened four times, a run cut short by reset) and checks, at
// every cycle, the rules of the word description in the paper. It never looks at the
// state encoding: any encoding and any number of states that behave as described pass.
// Compile and run (the course command):
//   iverilog -g2005 -Wall -o dishwasher.vvp dishwasher_tb.v dishwasher.v && vvp -n dishwasher.vvp
`timescale 1ns/1ns
module dishwasher_tb;
    reg        clk = 0;
    reg        reset = 1;
    reg        start = 0;
    reg        door_open = 0;
    wire       wash, rinse, dry, done;
    wire [2:0] state;
    integer    cycle;
    integer    errors = 0;
    // what the testbench knows about the machine from its own inputs (not from its state):
    // 0 = idle, 1 = a programme is running (possibly paused), 2 = done, waiting for the door
    integer    mode = 0;
    integer    n_wash = 0, n_rinse = 0, n_dry = 0;    // lamp cycles seen in this programme
    integer    phase = 0;                             // last phase lamp seen: 1 wash, 2 rinse, 3 dry
    integer    lamps;
    integer    this_phase;
    reg        w, r, d, f;                            // the lamps, with x counted as "not 1"
    integer    programmes = 0;

    dishwasher dut (.clk(clk), .reset(reset), .start(start), .door_open(door_open),
                    .wash(wash), .rinse(rinse), .dry(dry), .done(done), .state(state));

    always #5 clk = ~clk;

    task fail(input [8*64:1] why);
        begin
            $write("   <-- %0s", why);
            errors = errors + 1;
        end
    endtask

    initial begin
        $display("cycle reset start door | W R D F | state");
        for (cycle = 0; cycle < 77; cycle = cycle + 1) begin
            // inputs of this cycle, set half a period before the rising edge
            reset     = (cycle < 2) || (cycle == 56);
            start     = (cycle == 3) || (cycle == 18) || (cycle == 24) || (cycle == 27) ||
                        (cycle == 52) || (cycle == 60);
            door_open = (cycle == 23) || (cycle == 24) || (cycle == 29) || (cycle == 30) ||
                        (cycle == 35) || (cycle == 42) || (cycle == 47);
            @(posedge clk);
            #1;
            $write("%5d   %b     %b    %b   | %b %b %b %b |   %0d", cycle, reset, start, door_open,
                   wash, rinse, dry, done, state);
            w = (wash === 1'b1); r = (rinse === 1'b1); d = (dry === 1'b1); f = (done === 1'b1);
            if (^{wash, rinse, dry, done} === 1'bx) fail("a lamp is x (unknown)");
            lamps = w + r + d + f;
            this_phase = w ? 1 : r ? 2 : d ? 3 : 0;
            // rule 6: at most one lamp
            if (lamps > 1) fail("more than one lamp on");
            // rule 4 and 5: a sampled door_open = 1 never shows a phase lamp
            if (door_open && (w || r || d)) fail("phase lamp while the door is open");
            if (reset) begin
                if (lamps != 0) fail("lamp on after reset");
                mode = 0; n_wash = 0; n_rinse = 0; n_dry = 0; phase = 0;
            end else if (mode == 0) begin
                if (start && !door_open) begin
                    mode = 1; programmes = programmes + 1;
                    n_wash = 0; n_rinse = 0; n_dry = 0; phase = 0;
                    if (!w) fail("start accepted but wash lamp off");
                    else n_wash = 1; phase = 1;
                end else if (lamps != 0) fail("lamp on while idle");
            end else if (mode == 1) begin
                if (door_open) begin
                    if (lamps != 0) fail("lamp on while paused");
                end else if (f) begin
                    mode = 2;
                    if (n_wash != 6) fail("wrong number of wash cycles");
                    if (n_rinse != 4) fail("wrong number of rinse cycles");
                    if (n_dry != 3) fail("wrong number of dry cycles");
                end else if (this_phase == 0) begin
                    fail("no lamp on while running with the door closed");
                end else begin
                    if (this_phase < phase) fail("phase order broken");
                    phase = this_phase;
                    if (w) n_wash = n_wash + 1;
                    if (r) n_rinse = n_rinse + 1;
                    if (d) n_dry = n_dry + 1;
                end
            end else begin   // mode 2: done, waiting for the door
                if (door_open) begin
                    mode = 0;
                    if (lamps != 0) fail("lamp on after the door opened in DONE");
                end else if (!f || lamps != 1) fail("done lamp not held");
            end
            $write("\n");
        end
        if (programmes != 4) begin
            $display("programmes started: %0d (expected 4)", programmes);
            errors = errors + 1;
        end
        if (errors == 0)
            $display("RESULT: PASS (77 cycles, 4 programmes, no rule broken)");
        else
            $display("RESULT: FAIL (%0d rule violations)", errors);
        $finish(0);
    end
endmodule
