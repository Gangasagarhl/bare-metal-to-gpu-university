// The ticket-number display: it reads the counter once, just after every rising
// edge of clk, and shows the number. Compile with -DDUT=counter4 or -DDUT=counter_skip.
`timescale 1ns/1ns
module display_tb;
    reg        clk = 0;
    reg        reset = 1;
    reg        enable = 1;
    wire [3:0] count;
    wire       wrap;
    integer    edge_no = 0;
    integer    errors = 0;
    reg  [3:0] shown_before = 4'd0;

    `DUT dut (.clk(clk), .reset(reset), .enable(enable), .count(count), .wrap(wrap));

    always #5 clk = ~clk;        // period 10: rising edges at 5, 15, 25, ...

    initial begin
        $dumpfile("display.vcd");
        $dumpvars(0, display_tb);
        #12 reset = 0;           // reset is released between edges
    end

    always @(posedge clk) begin
        #1;
        edge_no = edge_no + 1;
        if (edge_no > 2 && count !== shown_before + 4'd1) begin
            $display("edge %2d: display shows %2d   <-- expected %0d", edge_no, count, shown_before + 4'd1);
            errors = errors + 1;
        end else begin
            $display("edge %2d: display shows %2d", edge_no, count);
        end
        shown_before = count;
        if (edge_no == 10) begin
            if (errors == 0)
                $display("RESULT: PASS (the display counted up by 1 at every edge)");
            else
                $display("RESULT: FAIL (%0d of 8 checked edges skipped a number)", errors);
            $finish(0);
        end
    end
endmodule
