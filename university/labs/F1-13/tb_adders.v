// Testbench: all 8 input rows of both full adders, checked against ordinary addition.
module tb_adders;
    reg a, b, cin;
    wire s, cout, s9, cout9;
    integer i;
    integer expected;                      // 32 bits wide, so 1 + 1 + 1 can be 3
    integer errors = 0;

    full_adder      fa(.a(a), .b(b), .cin(cin), .s(s),  .cout(cout));
    full_adder_nand fn(.a(a), .b(b), .cin(cin), .s(s9), .cout(cout9));

    initial begin
        $display("a b cin | cout s | a+b+cin | 9-NAND cout s");
        for (i = 0; i < 8; i = i + 1) begin
            {a, b, cin} = i[2:0];
            #1;
            expected = a + b + cin;
            $display("%b %b  %b  |  %b   %b |    %0d    |       %b    %b",
                     a, b, cin, cout, s, expected, cout9, s9);
            if ({cout, s}   !== expected[1:0]) errors = errors + 1;
            if ({cout9, s9} !== expected[1:0]) errors = errors + 1;
        end
        if (errors == 0) $display("ALL PASS (16 checks: 8 rows x 2 designs)");
        else $display("FAIL: %0d of 16 checks wrong", errors);
        $finish(0);
    end
endmodule
