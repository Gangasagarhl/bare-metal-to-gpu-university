// F1-25 Listing 2: the U16 control unit as combinational hardware (Verilog).
// Input: the 4-bit opcode. Outputs: the control signals of Listing 1, same names.
// alu encodes 0 = add, 1 = sub, 2 = and, 3 = or, 4 = slt.
module control (
    input  wire [3:0] op,
    output reg        reg_write, alu_imm, rd_as_b, mem_read, mem_write, mem_to_reg,
    output reg        br_eq, br_ne, out_en, halt,
    output reg  [2:0] alu
);
    always @(*) begin
        // defaults: do nothing, ALU adds
        {reg_write, alu_imm, rd_as_b, mem_read, mem_write, mem_to_reg} = 6'b000000;
        {br_eq, br_ne, out_en, halt} = 4'b0000;
        alu = 3'd0;
        case (op)
            4'd1:  begin reg_write = 1; alu = 3'd0; end                    // add
            4'd2:  begin reg_write = 1; alu = 3'd1; end                    // sub
            4'd3:  begin reg_write = 1; alu = 3'd2; end                    // and
            4'd4:  begin reg_write = 1; alu = 3'd3; end                    // or
            4'd5:  begin reg_write = 1; alu = 3'd4; end                    // slt
            4'd6:  begin reg_write = 1; alu_imm = 1; end                   // addi
            4'd7:  begin reg_write = 1; alu_imm = 1; mem_read = 1; mem_to_reg = 1; end // lw
            4'd8:  begin alu_imm = 1; rd_as_b = 1; mem_write = 1; end      // sw
            4'd9:  begin rd_as_b = 1; br_eq = 1; alu = 3'd1; end           // beq
            4'd10: begin rd_as_b = 1; br_ne = 1; alu = 3'd1; end           // bne
            4'd11: begin rd_as_b = 1; out_en = 1; end                      // out
            default: halt = 1;                                             // halt, 12..15
        endcase
    end
endmodule
