// Testbench for the F10 model answer (HW201 final): checks at every cycle the property
// "chime = 1 exactly when the door was sampled open at this edge and closed at the edge
// before", using a 2-bit shift register of door samples (the method of F1-19's Listing 2).
`timescale 1ns/1ns
module chime_tb;
    reg         clk = 0;
    reg         reset = 1;
    reg         door = 0;
    wire        chime;
    reg  [19:0] bits = 20'b0110_0011_1010_1110_0010;   // sent left bit first
    reg  [1:0]  last2 = 2'b00;                         // after reset the door counts as closed
    integer     i;
    integer     errors = 0;
    integer     chimes = 0;

    chime dut (.clk(clk), .reset(reset), .door(door), .chime(chime));

    always #5 clk = ~clk;

    initial begin
        @(posedge clk);
        #1 reset = 0;
        $display("cycle door last2 chime");
        for (i = 19; i >= 0; i = i - 1) begin
            door = bits[i];
            @(posedge clk);
            last2 = {last2[0], door};
            #1;
            $write("%5d   %b    %b     %b", 19 - i, door, last2, chime);
            if (chime !== (last2 == 2'b01)) begin
                $write("   <-- expected %b", (last2 == 2'b01));
                errors = errors + 1;
            end else if (chime) begin
                $write("   <- opened");
                chimes = chimes + 1;
            end
            $write("\n");
        end
        if (errors == 0)
            $display("RESULT: PASS (%0d chimes, one per opening, none while the door stayed open)", chimes);
        else
            $display("RESULT: FAIL (%0d mismatches)", errors);
        $finish(0);
    end
endmodule
