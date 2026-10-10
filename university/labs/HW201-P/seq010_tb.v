// Feed a fixed bit string into a 010 detector (-DDUT=seq010 or -DDUT=seq010_bug) and
// compare found with a direct check of the last three sampled bits (a shift register).
`timescale 1ns/1ns
module seq010_tb;
    reg         clk = 0;
    reg         reset = 1;
    reg         in = 0;
    wire        found;
    reg  [23:0] bits = 24'b0101_0010_1001_0101_1010_0100;   // sent left bit first
    reg  [2:0]  last3 = 3'b111;
    integer     i;
    integer     errors = 0;

    `DUT dut (.clk(clk), .reset(reset), .in(in), .found(found));

    always #5 clk = ~clk;

    initial begin
        @(posedge clk);
        #1 reset = 0;
        $display("cycle in last3 found");
        for (i = 23; i >= 0; i = i - 1) begin
            in = bits[i];
            @(posedge clk);
            last3 = {last3[1:0], in};
            #1;
            $write("%5d  %b   %b    %b", 23 - i, in, last3, found);
            if (found !== (last3 == 3'b010))
                begin $write("   <-- expected %b", (last3 == 3'b010)); errors = errors + 1; end
            else if (found)
                $write("   <- 010");
            $write("\n");
        end
        if (errors == 0)
            $display("RESULT: PASS (found = 1 exactly when the last three bits were 010)");
        else
            $display("RESULT: FAIL (%0d mismatches)", errors);
        $finish(0);
    end
endmodule
