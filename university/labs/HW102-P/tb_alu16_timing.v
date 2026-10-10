// Project reference timing report (F1-15's model: 1 time unit per gate, compiled with
// -DTIMING). Measures when the last output of the 16-bit ALU changes after an input change.
module tb_alu16_timing;
    parameter N = 16;
    reg [N-1:0] a, b;
    reg [2:0] op;
    wire [N-1:0] y;
    wire zero, negative, carry, overflow;
    integer last, t0;

    alu_n #(N) dut(.a(a), .b(b), .op(op), .y(y), .zero(zero), .negative(negative),
                   .carry(carry), .overflow(overflow));

    always @(y or zero or negative or carry or overflow) last = $time;

    task settle;                            // let everything settle, then mark t0
        begin
            #300;
            t0 = $time;
        end
    endtask

    task report;
        input [8*72-1:0] label;
        begin
            #300;
            $display("%0s: last output change %0d units after the input change", label, last - t0);
        end
    endtask

    initial begin
        $display("16-bit structural ALU, 1 model time unit per gate (not nanoseconds)");
        op = 3'b000; a = 16'hFFFF; b = 16'h0000; settle;
        b = 16'h0001;              report("ADD  FFFF + 0 -> FFFF + 1 (carry through all 16 bits)");
        op = 3'b000; a = 16'h0000; b = 16'h0000; settle;
        b = 16'h0001;              report("ADD  0000 + 0 -> 0000 + 1 (no carry chain)         ");
        op = 3'b001; a = 16'h0000; b = 16'h0000; settle;
        b = 16'h0001;              report("SUB  0000 - 0 -> 0000 - 1 (borrow through all bits) ");
        op = 3'b000; a = 16'h00FF; b = 16'h0001; settle;
        op = 3'b100;               report("ADD -> XOR, operands fixed (opcode change only)     ");
        op = 3'b100; a = 16'h00FF; b = 16'h0001; settle;
        op = 3'b001;               report("XOR -> SUB, operands fixed (opcode change only)     ");
        $finish(0);
    end
endmodule
