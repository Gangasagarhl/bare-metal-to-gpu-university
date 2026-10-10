// Testbench: the XOR alone, all four input pairs, with the inside wires shown.
module tb_xor;
    reg a, b;
    wire y;
    integer i;
    integer errors = 0;

    xor_n dut(.a(a), .b(b), .y(y));

    initial begin
        $display("a b | t u v | y | expected a^b");
        for (i = 0; i < 4; i = i + 1) begin
            {a, b} = i[1:0];
            #1;
            $display("%b %b | %b %b %b | %b | %b", a, b, dut.t, dut.u, dut.v, y, a ^ b);
            if (y !== (a ^ b)) errors = errors + 1;
        end
        if (errors == 0) $display("ALL PASS (4 vectors)");
        else $display("FAIL: %0d of 4 vectors wrong", errors);
        $finish(0);
    end
endmodule
