// Self-checking testbench for a traffic light: compile with -DDUT=traffic_light
// (or with the forensic evidence, -DDUT=traffic_bug). It prints one line per clock
// cycle and checks three safety rules at every cycle.
`timescale 1ns/1ns
module traffic_tb;
    reg        clk = 0;
    reg        reset = 1;
    reg        car_ew = 0;
    wire [1:0] ns;
    wire [1:0] ew;
    wire [2:0] state;
    integer    cycle;
    integer    errors = 0;
    reg  [1:0] ns_before = 2'd0;
    reg  [1:0] ew_before = 2'd0;

    `DUT dut (.clk(clk), .reset(reset), .car_ew(car_ew), .ns(ns), .ew(ew), .state(state));

    always #5 clk = ~clk;

    function [7:0] colour;            // one letter per light: R, Y or G
        input [1:0] light;
        colour = (light == 2'd0) ? "R" : (light == 2'd1) ? "Y" : (light == 2'd2) ? "G" : "?";
    endfunction

    initial begin
        $display("cycle car_ew state  NS EW");
        for (cycle = 0; cycle < 40; cycle = cycle + 1) begin
            reset  = (cycle < 2);
            // cars arrive on the side road in cycles 4-13 and 27-31
            car_ew = (cycle >= 4 && cycle <= 13) || (cycle >= 27 && cycle <= 31);
            @(posedge clk);
            #1;
            $write("%5d    %b     %0d    %s  %s", cycle, car_ew, state, colour(ns), colour(ew));
            // rule 1: never green or yellow on both roads at once
            if (ns != 2'd0 && ew != 2'd0) begin
                $write("   <-- both roads open");
                errors = errors + 1;
            end
            // rule 2: a green light may only turn into yellow (never straight to red)
            if ((ns_before == 2'd2 && ns == 2'd0) || (ew_before == 2'd2 && ew == 2'd0)) begin
                $write("   <-- green went straight to red");
                errors = errors + 1;
            end
            // rule 3: a road may turn green only after a cycle in which both were red
            if (!reset && ((ns == 2'd2 && ns_before != 2'd2 && ew_before != 2'd0) ||
                           (ew == 2'd2 && ew_before != 2'd2 && ns_before != 2'd0))) begin
                $write("   <-- green without an all-red cycle first");
                errors = errors + 1;
            end
            $write("\n");
            ns_before = ns;
            ew_before = ew;
        end
        if (errors == 0)
            $display("RESULT: PASS (40 cycles, no safety rule broken)");
        else
            $display("RESULT: FAIL (%0d safety rule violations)", errors);
        $finish(0);
    end
endmodule
