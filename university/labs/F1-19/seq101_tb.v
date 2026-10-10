// Feed a fixed bit string into seq101 and compare found with a direct check of the
// last three bits.
`timescale 1ns/1ns
module seq101_tb;
    reg         clk = 0;
    reg         reset = 1;
    reg         in = 0;
    wire        found;
    reg  [19:0] bits = 20'b1010_1100_1011_0101_0010;   // sent left bit first
    reg  [2:0]  last3 = 3'b000;
    integer     i;
    integer     errors = 0;

    seq101 dut (.clk(clk), .reset(reset), .in(in), .found(found));

    always #5 clk = ~clk;

    initial begin
        @(posedge clk);
        #1 reset = 0;
        $display(" i in last3 found");
        for (i = 19; i >= 0; i = i - 1) begin
            in = bits[i];
            @(posedge clk);
            last3 = {last3[1:0], in};
            #1;
            if (found)
                $display("%2d  %b   %b    %b  <- 101", 19 - i, in, last3, found);
            else
                $display("%2d  %b   %b    %b", 19 - i, in, last3, found);
            if (found !== (last3 == 3'b101))
                errors = errors + 1;
        end
        if (errors == 0)
            $display("RESULT: PASS (found = 1 exactly when the last three bits were 101)");
        else
            $display("RESULT: FAIL (%0d mismatches)", errors);
        $finish(0);
    end
endmodule
