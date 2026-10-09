// Testbench for the N-bit ALU: a reference model computes every expected result and flag.
// N = 4: all 2048 combinations. Larger N: edge cases plus 200000 pseudo-random vectors.
module tb_alu;
    parameter N = 4;
    reg [N-1:0] a, b;
    reg [2:0] op;
    wire [N-1:0] y;
    wire zero, negative, carry, overflow;
    integer i, seed, errors, checks;

    alu #(N) dut(.a(a), .b(b), .op(op), .y(y), .zero(zero), .negative(negative),
                 .carry(carry), .overflow(overflow));

    // Reference model: ordinary integer arithmetic, no gates.
    task check;
        reg [N:0] full;
        reg [N-1:0] ry;
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

    task show;                              // print one named example
        begin
            #1;
            check;
            $display("op=%b a=%h b=%h -> y=%h  Z=%b N=%b C=%b V=%b", op, a, b, y,
                     zero, negative, carry, overflow);
        end
    endtask

    initial begin
        errors = 0; checks = 0; seed = 102;
        $display("ALU width N = %0d", N);
        op = 3'b000; a = 7;  b = 1;  show;     // 7 + 1
        op = 3'b000; a = {N{1'b1}}; b = 1; show; // all ones + 1
        op = 3'b001; a = 5;  b = 7;  show;     // 5 - 7
        op = 3'b001; a = 7;  b = 7;  show;     // 7 - 7
        op = 3'b001; a = {1'b1, {(N-1){1'b0}}}; b = 1; show; // most negative - 1
        op = 3'b010; a = 12; b = 10; show;     // 1100 AND 1010
        op = 3'b100; a = 12; b = 10; show;     // 1100 XOR 1010
        if (N <= 8) begin
            for (i = 0; i < (8 << (2 * N)); i = i + 1) begin
                {op, a, b} = i;
                #1;
                check;
            end
        end else begin
            for (i = 0; i < 200000; i = i + 1) begin
                a = $random(seed);
                b = $random(seed);
                op = $random(seed);
                #1;
                check;
            end
        end
        if (errors == 0) $display("ALL PASS (%0d vectors)", checks);
        else $display("FAIL: %0d of %0d vectors wrong", errors, checks);
        $finish(0);
    end
endmodule
