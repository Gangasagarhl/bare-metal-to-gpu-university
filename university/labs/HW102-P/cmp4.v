// Reference for final question F19 (design): a 4-bit compare block built on the ALU's
// SUB operation and its flags. eq = Z; lt_unsigned = NOT C (a borrow was needed);
// lt_signed = N XOR V (the sign bit is right when there is no overflow, flipped when there is).
module cmp4(input wire [3:0] a, input wire [3:0] b,
            output wire eq, output wire lt_unsigned, output wire lt_signed);
    wire [3:0] y;
    wire zero, negative, carry, overflow;
    alu4 sub(.a(a), .b(b), .op(3'b001), .y(y), .zero(zero), .negative(negative),
             .carry(carry), .overflow(overflow));
    assign eq = zero;
    not g_lt(lt_unsigned, carry);
    xor g_ls(lt_signed, negative, overflow);
endmodule
