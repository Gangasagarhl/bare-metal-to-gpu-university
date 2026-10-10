// Testbench: compare every NAND-built gate with Verilog's own operators, for all inputs.
module tb_nand_only;
    reg a, b;
    wire y_not, y_and, y_or, y_nor, y_xor, y_xnor;
    integer i;
    integer errors = 0;

    not_n  g1(.a(a), .y(y_not));
    and_n  g2(.a(a), .b(b), .y(y_and));
    or_n   g3(.a(a), .b(b), .y(y_or));
    nor_n  g4(.a(a), .b(b), .y(y_nor));
    xor_n  g5(.a(a), .b(b), .y(y_xor));
    xnor_n g6(.a(a), .b(b), .y(y_xnor));

    initial begin
        $display("a b | NOT a AND OR NOR XOR XNOR");
        for (i = 0; i < 4; i = i + 1) begin
            {a, b} = i[1:0];
            #1;
            $display("%b %b |   %b    %b   %b   %b   %b   %b",
                     a, b, y_not, y_and, y_or, y_nor, y_xor, y_xnor);
            if (y_not  !== ~a)       errors = errors + 1;
            if (y_and  !== (a & b))  errors = errors + 1;
            if (y_or   !== (a | b))  errors = errors + 1;
            if (y_nor  !== ~(a | b)) errors = errors + 1;
            if (y_xor  !== (a ^ b))  errors = errors + 1;
            if (y_xnor !== ~(a ^ b)) errors = errors + 1;
        end
        if (errors == 0) $display("ALL PASS (24 checks)");
        else $display("FAIL: %0d of 24 checks wrong", errors);
        $finish(0);
    end
endmodule
