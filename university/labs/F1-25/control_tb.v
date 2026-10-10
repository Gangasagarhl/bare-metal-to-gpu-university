// F1-25 Listing 3: test bench. Applies every opcode 0..15 and prints the outputs in the
// same columns as Listing 1, so the two tables can be compared line by line with diff.
module control_tb;
    reg  [3:0] op;
    wire reg_write, alu_imm, rd_as_b, mem_read, mem_write, mem_to_reg;
    wire br_eq, br_ne, out_en, halt;
    wire [2:0] alu;
    integer i;
    control dut (.op(op), .reg_write(reg_write), .alu_imm(alu_imm), .rd_as_b(rd_as_b),
                 .mem_read(mem_read), .mem_write(mem_write), .mem_to_reg(mem_to_reg),
                 .br_eq(br_eq), .br_ne(br_ne), .out_en(out_en), .halt(halt), .alu(alu));
    initial begin
        for (i = 0; i < 16; i = i + 1) begin
            op = i;
            #1;  // let the combinational logic settle
            $display("%2d %5d %6d %5d %5d %5d %8d %4d %4d %3d %4d %0d", op, reg_write,
                     alu_imm, rd_as_b, mem_read, mem_write, mem_to_reg, br_eq, br_ne,
                     out_en, halt, alu);
        end
        $finish;
    end
endmodule
