// Send 1, 2, 3, ... into the delay line, one value per edge. The value at "in" at
// edge n passes three flip-flops (at edges n, n+1, n+2), so just after edge n
// "out" must show the value sent at edge n-2. Compile with -DDUT=delay_line or -DDUT=delay_line_bug.
`timescale 1ns/1ns
module delay_tb;
    reg        clk = 0;
    reg  [3:0] in = 4'd0;
    wire [3:0] out;
    integer    n;
    integer    errors = 0;

    `DUT dut (.clk(clk), .in(in), .out(out));

    always #5 clk = ~clk;

    initial begin
        $display("edge  in  out  (expected out)");
        for (n = 1; n <= 8; n = n + 1) begin
            in = n;
            @(posedge clk); #1;
            if (n <= 2) begin
                $display("%4d  %2d  %3d  (not yet defined)", n, in, out);
            end else if (out === n - 2) begin
                $display("%4d  %2d  %3d  (%0d)", n, in, out, n - 2);
            end else begin
                $display("%4d  %2d  %3d  (%0d)  <-- wrong", n, in, out, n - 2);
                errors = errors + 1;
            end
        end
        if (errors == 0)
            $display("RESULT: PASS (every value passed three stages)");
        else
            $display("RESULT: FAIL (%0d of 6 checked edges wrong)", errors);
        $finish(0);
    end
endmodule
