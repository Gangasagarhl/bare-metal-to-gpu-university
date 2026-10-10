// Fill all 16 words, read them back, then check that we = 0 really protects them.
`timescale 1ns/1ns
module ram_tb;
    reg        clk = 0;
    reg        we = 0;
    reg  [3:0] addr = 4'd0;
    reg  [7:0] din = 8'd0;
    wire [7:0] dout;
    integer    i;
    integer    errors = 0;

    ram16x8 ram (.clk(clk), .we(we), .addr(addr), .din(din), .dout(dout));

    always #5 clk = ~clk;

    function [7:0] pattern;         // the value written to address a
        input [3:0] a;
        pattern = {a, ~a};          // for example address 3 -> 0011_1100
    endfunction

    initial begin
        // 1. write every address
        for (i = 0; i < 16; i = i + 1) begin
            addr = i; din = pattern(i); we = 1;
            @(posedge clk); #1;
        end
        we = 0;
        // 2. read every address back
        $display("addr  dout      expected");
        for (i = 0; i < 16; i = i + 1) begin
            addr = i; #1;
            $display("%4d  %b  %b", addr, dout, pattern(i));
            if (dout !== pattern(i)) errors = errors + 1;
        end
        // 3. with we = 0, a clock edge must not change address 5
        addr = 5; din = 8'hFF; we = 0;
        @(posedge clk); #1;
        $display("after an edge with we = 0 and din = 11111111: mem[5] = %b", dout);
        if (dout !== pattern(5)) errors = errors + 1;
        if (errors == 0)
            $display("RESULT: PASS (16 words written and read back; we = 0 protects)");
        else
            $display("RESULT: FAIL (%0d errors)", errors);
        $finish(0);
    end
endmodule
