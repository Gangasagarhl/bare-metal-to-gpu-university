// Dana's testbench (forensic evidence for the HW102 final): named vectors for every
// opcode with expected and observed result and flags, then the number of wrong vectors
// per opcode over all 256 (a, b) pairs. The reference model is plain integer arithmetic.
module tb_alu4_forensic;
    reg [3:0] a, b;
    reg [2:0] op;
    wire [3:0] y;
    wire zero, negative, carry, overflow;
    integer i, j, wrong, total;
    reg [4:0] full;
    reg [3:0] ry;
    reg rc, rv;
    integer sa, sb, sr;
    reg [47:0] opname [0:7];

    alu4 dut(.a(a), .b(b), .op(op), .y(y), .zero(zero), .negative(negative),
             .carry(carry), .overflow(overflow));

    task model;                               // expected result and flags
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
            ry = full[3:0];
            if (op == 3'b000 || op == 3'b001) begin
                rc = full[4];
                rv = (sr > 7) || (sr < -8);
            end
        end
    endtask

    function mismatch;
        input dummy;
        begin
            mismatch = (y !== ry || zero !== (ry == 0) || negative !== ry[3] ||
                        carry !== rc || overflow !== rv);
        end
    endfunction

    task named;
        input [2:0] o; input [3:0] va, vb;
        begin
            op = o; a = va; b = vb;
            #1;
            model;
            $display("%b %s | a=%b b=%b | expected y=%b ZNCV=%b%b%b%b | got y=%b ZNCV=%b%b%b%b | %s",
                     op, opname[op], a, b, ry, (ry == 0), ry[3], rc, rv,
                     y, zero, negative, carry, overflow, mismatch(0) ? "WRONG" : "ok");
        end
    endtask

    initial begin
        opname[0] = "ADD   "; opname[1] = "SUB   "; opname[2] = "AND   "; opname[3] = "OR    ";
        opname[4] = "XOR   "; opname[5] = "NOT a "; opname[6] = "PASS a"; opname[7] = "PASS b";
        $display("Dana's ALU: named vectors (op, a, b in binary)");
        named(3'b000, 4'b1001, 4'b0111);      // ADD 9 + 7
        named(3'b000, 4'b0110, 4'b0010);      // ADD 6 + 2
        named(3'b001, 4'b0101, 4'b0111);      // SUB 5 - 7
        named(3'b001, 4'b1001, 4'b0011);      // SUB 9 - 3
        named(3'b010, 4'b1010, 4'b0101);      // AND
        named(3'b010, 4'b1100, 4'b1010);      // AND
        named(3'b011, 4'b1010, 4'b0101);      // OR
        named(3'b011, 4'b0110, 4'b0001);      // OR
        named(3'b100, 4'b0111, 4'b0101);      // XOR
        named(3'b100, 4'b0011, 4'b0101);      // XOR
        named(3'b101, 4'b0110, 4'b0000);      // NOT a
        named(3'b101, 4'b1111, 4'b1010);      // NOT a
        named(3'b110, 4'b0101, 4'b1100);      // PASS a
        named(3'b110, 4'b0011, 4'b0100);      // PASS a
        named(3'b111, 4'b0101, 4'b1100);      // PASS b
        named(3'b111, 4'b0000, 4'b1000);      // PASS b
        $display("wrong vectors per opcode, over all 256 (a, b) pairs:");
        total = 0;
        for (i = 0; i < 8; i = i + 1) begin
            op = i[2:0];
            wrong = 0;
            for (j = 0; j < 256; j = j + 1) begin
                {a, b} = j[7:0];
                #1;
                model;
                if (mismatch(0)) wrong = wrong + 1;
            end
            total = total + wrong;
            $display("  op=%b %s: %3d of 256 wrong", op, opname[i], wrong);
        end
        if (total == 0) $display("ALL PASS (2048 vectors)");
        else $display("FAIL: %0d of 2048 vectors wrong", total);
        $finish(0);
    end
endmodule
