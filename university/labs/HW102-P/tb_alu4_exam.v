// HW102 practical exam (P) - the exam testbench (given to candidates, do not change).
// A reference model (plain integer arithmetic, no gates) computes the expected result
// and flags; eight named vectors are printed, then all 2048 input combinations
// (16 values of a x 16 values of b x 8 opcodes) are checked.
module tb_alu4_exam;
    reg [3:0] a, b;
    reg [2:0] op;
    wire [3:0] y;
    wire zero, negative, carry, overflow;
    integer i, errors, checks;

    alu4 dut(.a(a), .b(b), .op(op), .y(y), .zero(zero), .negative(negative),
             .carry(carry), .overflow(overflow));

    task check;                              // compare with the reference model
        reg [4:0] full;
        reg [3:0] ry;
        reg rc, rv;
        integer sa, sb, sr;
        begin
            sa = $signed(a);
            sb = $signed(b);
            rc = 0; rv = 0; sr = 0;
            case (op)
                3'b000: begin full = {1'b0, a} + {1'b0, b};      sr = sa + sb; end
                3'b001: begin full = {1'b0, a} + {1'b0, ~b} + 1; sr = sa - sb; end
                3'b010: full = {1'b0, a & b};
                3'b011: full = {1'b0, a | b};
                3'b100: full = {1'b0, a ^ b};
                3'b101: full = {1'b0, ~a};
                3'b110: full = {1'b0, a};
                default: full = {1'b0, b};
            endcase
            ry = full[3:0];
            if (op == 3'b000 || op == 3'b001) begin
                rc = full[4];
                rv = (sr > 7) || (sr < -8);
            end
            checks = checks + 1;
            if (y !== ry || zero !== (ry == 0) || negative !== ry[3] ||
                carry !== rc || overflow !== rv) begin
                errors = errors + 1;
                if (errors <= 5)
                    $display("MISMATCH op=%b a=%h b=%h: got y=%h ZNCV=%b%b%b%b, expected y=%h ZNCV=%b%b%b%b",
                             op, a, b, y, zero, negative, carry, overflow,
                             ry, (ry == 0), ry[3], rc, rv);
            end
        end
    endtask

    task show;                               // print one named example
        begin
            #1;
            check;
            $display("op=%b a=%h b=%h -> y=%h  Z=%b N=%b C=%b V=%b", op, a, b, y,
                     zero, negative, carry, overflow);
        end
    endtask

    initial begin
        errors = 0; checks = 0;
        $display("HW102 practical: 4-bit ALU against the reference model");
        op = 3'b000; a = 4'h9; b = 4'h7; show;    // 9 + 7 (unsigned carry out)
        op = 3'b000; a = 4'h6; b = 4'h2; show;    // 6 + 2 (signed overflow)
        op = 3'b001; a = 4'h3; b = 4'h9; show;    // 3 - 9
        op = 3'b001; a = 4'h8; b = 4'h8; show;    // 8 - 8
        op = 3'b001; a = 4'h0; b = 4'h1; show;    // 0 - 1
        op = 3'b011; a = 4'hA; b = 4'h5; show;    // 1010 OR 0101
        op = 3'b101; a = 4'h6; b = 4'h0; show;    // NOT 0110
        op = 3'b111; a = 4'h0; b = 4'hC; show;    // PASS b
        for (i = 0; i < 2048; i = i + 1) begin
            {op, a, b} = i[10:0];
            #1;
            check;
        end
        if (errors == 0) $display("ALL PASS (%0d vectors)", checks);
        else $display("FAIL: %0d of %0d vectors wrong", errors, checks);
        $finish(0);
    end
endmodule
