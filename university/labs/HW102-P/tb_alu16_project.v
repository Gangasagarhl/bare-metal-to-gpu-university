// Project reference test: the 16-bit structural ALU against a reference model.
// Eight named edge cases, then 100000 pseudo-random vectors from a fixed seed (1602).
module tb_alu16_project;
    parameter N = 16;
    reg [N-1:0] a, b;
    reg [2:0] op;
    wire [N-1:0] y;
    wire zero, negative, carry, overflow;
    integer i, seed, errors, checks;

    alu_n #(N) dut(.a(a), .b(b), .op(op), .y(y), .zero(zero), .negative(negative),
                   .carry(carry), .overflow(overflow));

    task check;
        reg [N:0] full;
        reg [N-1:0] ry;
        reg rc, rv;
        integer sa, sb, sr;
        begin
            sa = $signed(a); sb = $signed(b); rc = 0; rv = 0; sr = 0;
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
            ry = full[N-1:0];
            if (op == 3'b000 || op == 3'b001) begin
                rc = full[N];
                rv = (sr > (2 ** (N - 1)) - 1) || (sr < -(2 ** (N - 1)));
            end
            checks = checks + 1;
            if (y !== ry || zero !== (ry == 0) || negative !== ry[N-1] ||
                carry !== rc || overflow !== rv) begin
                errors = errors + 1;
                if (errors <= 5)
                    $display("MISMATCH op=%b a=%h b=%h: got y=%h ZNCV=%b%b%b%b, expected y=%h ZNCV=%b%b%b%b",
                             op, a, b, y, zero, negative, carry, overflow,
                             ry, (ry == 0), ry[N-1], rc, rv);
            end
        end
    endtask

    task show;
        begin
            #1;
            check;
            $display("op=%b a=%h b=%h -> y=%h  Z=%b N=%b C=%b V=%b", op, a, b, y,
                     zero, negative, carry, overflow);
        end
    endtask

    initial begin
        errors = 0; checks = 0; seed = 1602;
        $display("project reference ALU, N = %0d", N);
        op = 3'b000; a = 16'hFFFF; b = 16'h0001; show;   // all ones + 1: long carry, C = 1
        op = 3'b000; a = 16'h7FFF; b = 16'h0001; show;   // most positive + 1: V = 1
        op = 3'b000; a = 16'h8000; b = 16'h8000; show;   // most negative + itself: Z, C, V
        op = 3'b001; a = 16'h8000; b = 16'h0001; show;   // most negative - 1: V = 1
        op = 3'b001; a = 16'h0000; b = 16'h0001; show;   // 0 - 1: borrow, N = 1
        op = 3'b001; a = 16'h1234; b = 16'h1234; show;   // a - a: Z = 1, C = 1
        op = 3'b100; a = 16'hAAAA; b = 16'h5555; show;   // XOR to all ones: N = 1, C = V = 0
        op = 3'b101; a = 16'hFFFF; b = 16'h0000; show;   // NOT all ones: Z = 1
        for (i = 0; i < 100000; i = i + 1) begin
            a = $random(seed);
            b = $random(seed);
            op = $random(seed);
            #1;
            check;
        end
        if (errors == 0) $display("ALL PASS (%0d vectors)", checks);
        else $display("FAIL: %0d of %0d vectors wrong", errors, checks);
        $finish(0);
    end
endmodule
