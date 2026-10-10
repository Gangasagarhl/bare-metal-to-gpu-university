// Run the slow-adder counter with a clock period chosen at compile time
// (-DPERIOD=10 or -DPERIOD=6) and check it against a model that adds exactly 1
// at every rising edge.
`timescale 1ns/1ns
module clock_speed_tb;
    reg        clk = 0;
    reg        reset = 1;
    wire [3:0] count;
    integer    edge_no = 0;
    integer    errors = 0;
    reg  [3:0] model = 4'd0;     // what count should be

    slow_adder_counter dut (.clk(clk), .reset(reset), .count(count));

    always #(`PERIOD / 2) clk = ~clk;      // first rising edge at PERIOD / 2

    initial begin
        $dumpfile("clock_speed.vcd");
        $dumpvars(0, clock_speed_tb);
        $display("clock period = %0d time units, adder delay = 7 time units", `PERIOD);
        #(2 * `PERIOD + 1) reset = 0;      // two rising edges with reset = 1
    end

    always @(posedge clk) begin
        if (reset)
            model = 4'd0;
        else
            model = model + 4'd1;
        #1;
        edge_no = edge_no + 1;
        if (count !== model) begin
            $display("edge %2d at t=%3t: count = %2d   <-- should be %0d", edge_no, $time - 1, count, model);
            errors = errors + 1;
        end else begin
            $display("edge %2d at t=%3t: count = %2d", edge_no, $time - 1, count);
        end
        if (edge_no == 12) begin
            if (errors == 0)
                $display("RESULT: PASS (count went up by 1 at every edge)");
            else
                $display("RESULT: FAIL (%0d of 12 edges wrong)", errors);
            $finish(0);
        end
    end
endmodule
