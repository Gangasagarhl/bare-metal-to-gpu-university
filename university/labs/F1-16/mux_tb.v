// Self-checking testbench for the selector. Compile it with mux_bug.v or with
// mux_fixed.v; both define a module that is renamed to "mux_dut" by the -D option.
`timescale 1ns/1ns
module mux_tb;
    reg  [1:0] sel;
    reg        a;
    reg        b;
    reg        c;
    wire       y;
    integer    i;
    integer    errors = 0;
    reg        expected;

    `DUT dut (.sel(sel), .a(a), .b(b), .c(c), .y(y));

    initial begin
        $display(" sel a b c | y expected");
        // 32 input patterns: every sel with every a, b, c, in counting order
        for (i = 0; i < 32; i = i + 1) begin
            {a, b, c, sel} = i[4:0];
            #1;
            case (sel)
                2'd0: expected = a;
                2'd1: expected = b;
                2'd2: expected = c;
                default: expected = 1'b0;
            endcase
            // print the rows where sel = 3, and every wrong row
            if (y !== expected) begin
                $display("  %0d  %b %b %b | %b %b   <-- wrong", sel, a, b, c, y, expected);
                errors = errors + 1;
            end else if (sel == 2'd3) begin
                $display("  %0d  %b %b %b | %b %b", sel, a, b, c, y, expected);
            end
        end
        if (errors == 0)
            $display("RESULT: PASS (32 patterns)");
        else
            $display("RESULT: FAIL (%0d of 32 patterns wrong)", errors);
        $finish(0);
    end
endmodule
