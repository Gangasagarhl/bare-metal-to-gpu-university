// Self-checking testbench for register4, counter4 and shift4.
`timescale 1ns/1ns
module regs_tb;
    reg        clk = 0;
    reg        reset = 1;
    reg        load = 0;
    reg  [3:0] d = 4'd0;
    reg        enable = 0;
    reg        serial_in = 0;
    wire [3:0] q_reg;
    wire [3:0] count;
    wire       wrap;
    wire [3:0] q_shift;
    integer    errors = 0;
    integer    cycle;
    reg  [3:0] model_reg;    // what each part should hold, computed by the testbench
    reg  [3:0] model_count;
    reg  [3:0] model_shift;
    reg  [7:0] bits = 8'b1011_0010;   // serial input, sent left bit first

    register4 r (.clk(clk), .reset(reset), .load(load), .d(d), .q(q_reg));
    counter4  c (.clk(clk), .reset(reset), .enable(enable), .count(count), .wrap(wrap));
    shift4    s (.clk(clk), .serial_in(serial_in), .q(q_shift));

    always #5 clk = ~clk;    // rising edges at 5, 15, 25, ...

    initial begin
        $display("cycle reset load  d  | q_reg | enable count wrap | in q_shift");
        model_shift = 4'bxxxx;
        for (cycle = 0; cycle < 20; cycle = cycle + 1) begin
            // change the inputs half a period before the rising edge
            reset     = (cycle == 0);
            load      = (cycle == 2 || cycle == 6);
            d         = (cycle == 2) ? 4'd9 : 4'd6;
            enable    = (cycle != 4 && cycle != 5);
            serial_in = (cycle < 8) ? bits[7 - cycle] : 1'b0;
            @(posedge clk);
            // the models: exactly the rules written in the modules' comments
            if (reset) model_reg = 4'd0; else if (load) model_reg = d;
            if (reset) model_count = 4'd0; else if (enable) model_count = model_count + 4'd1;
            model_shift = {model_shift[2:0], serial_in};
            #1;
            $display("%5d   %b    %b   %2d  |  %2d   |   %b     %2d    %b  |  %b  %b",
                     cycle, reset, load, d, q_reg, enable, count, wrap, serial_in, q_shift);
            if (q_reg !== model_reg || count !== model_count)
                errors = errors + 1;
            if (cycle >= 3 && q_shift !== model_shift)
                errors = errors + 1;
            #3;
        end
        if (errors == 0)
            $display("RESULT: PASS (20 cycles, all three parts match their rules)");
        else
            $display("RESULT: FAIL (%0d mismatches)", errors);
        $finish(0);
    end
endmodule
