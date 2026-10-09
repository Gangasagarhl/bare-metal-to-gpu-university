// Testbench: every input pair for NOT, NAND and NOR, compared with Verilog's own operators.
module tb_cmos_gates;
    reg a, b;
    wire y_not, y_nand, y_nor;
    integer i;
    integer errors = 0;

    cmos_not   g1(.a(a), .y(y_not));
    cmos_nand2 g2(.a(a), .b(b), .y(y_nand));
    cmos_nor2  g3(.a(a), .b(b), .y(y_nor));

    initial begin
        $display("a b | NOT a | NAND | NOR");
        for (i = 0; i < 4; i = i + 1) begin
            {a, b} = i[1:0];
            #1;
            $display("%b %b |   %b   |  %b   |  %b", a, b, y_not, y_nand, y_nor);
            if (y_not  !== ~a)       errors = errors + 1;
            if (y_nand !== ~(a & b)) errors = errors + 1;
            if (y_nor  !== ~(a | b)) errors = errors + 1;
        end
        if (errors == 0) $display("ALL PASS (12 checks)");
        else $display("FAIL: %0d of 12 checks wrong", errors);
        $finish(0);
    end
endmodule
