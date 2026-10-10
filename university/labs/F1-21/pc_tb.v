// Self-checking testbench for program_counter: a script of inputs, and the value pc must have
// after each rising edge.
`timescale 1ns/1ns
module pc_tb;
    reg        clk = 0;
    reg        reset = 0;
    reg        load = 0;
    reg        inc = 0;
    reg  [7:0] jump_to = 8'd0;
    wire [7:0] pc_value;
    integer    errors = 0;

    program_counter #(.WIDTH(8)) dut (.clk(clk), .reset(reset), .load(load), .inc(inc),
                         .jump_to(jump_to), .pc(pc_value));

    always #5 clk = ~clk;

    task step(input r, input l, input i, input [7:0] j, input [7:0] expected);
        begin
            reset = r; load = l; inc = i; jump_to = j;
            @(posedge clk); #1;
            if (pc_value === expected) begin
                $display("reset=%b load=%b inc=%b jump_to=%3d -> pc=%3d", r, l, i, j, pc_value);
            end else begin
                $display("reset=%b load=%b inc=%b jump_to=%3d -> pc=%3d  <-- expected %0d",
                         r, l, i, j, pc_value, expected);
                errors = errors + 1;
            end
        end
    endtask

    initial begin
        step(1, 0, 0,   0,   0);     // reset
        step(0, 0, 1,   0,   1);     // count
        step(0, 0, 1,   0,   2);
        step(0, 0, 0,   0,   2);     // hold
        step(0, 1, 1,  40,  40);     // load wins over inc
        step(0, 0, 1,   0,  41);
        step(1, 1, 1,  99,   0);     // reset wins over everything
        step(0, 1, 0, 254, 254);
        step(0, 0, 1,   0, 255);
        step(0, 0, 1,   0,   0);     // 255 + 1 wraps to 0 in 8 bits
        if (errors == 0)
            $display("RESULT: PASS (10 steps)");
        else
            $display("RESULT: FAIL (%0d wrong steps)", errors);
        $finish(0);
    end
endmodule
