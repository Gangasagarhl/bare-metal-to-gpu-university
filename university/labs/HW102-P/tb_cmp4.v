// Exhaustive test of cmp4 against integer comparison (all 256 pairs).
module tb_cmp4;
    reg [3:0] a, b;
    wire eq, lt_unsigned, lt_signed;
    integer i, errors, ua, ub, sa, sb;

    cmp4 dut(.a(a), .b(b), .eq(eq), .lt_unsigned(lt_unsigned), .lt_signed(lt_signed));

    initial begin
        errors = 0;
        $display("   a    b   | eq ltu lts");
        for (i = 0; i < 256; i = i + 1) begin
            {a, b} = i[7:0];
            #1;
            ua = a; ub = b; sa = $signed(a); sb = $signed(b);
            if (eq !== (ua == ub) || lt_unsigned !== (ua < ub) || lt_signed !== (sa < sb))
                errors = errors + 1;
            if (i == 8'h00 || i == 8'h37 || i == 8'h73 || i == 8'h8F || i == 8'hF8 || i == 8'h2E || i == 8'hE2)
                $display("%b %b |  %b   %b   %b   (unsigned %2d vs %2d, signed %2d vs %2d)",
                         a, b, eq, lt_unsigned, lt_signed, ua, ub, sa, sb);
        end
        if (errors == 0) $display("ALL PASS (256 pairs, 768 checks)");
        else $display("FAIL: %0d of 256 pairs wrong", errors);
        $finish(0);
    end
endmodule
