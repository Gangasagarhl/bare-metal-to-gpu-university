// Demonstration of a testbench trap: Verilog sizes an expression by its operands.
module width_demo;
    reg a = 1'b1, b = 1'b1, cin = 1'b1;     // three 1-bit values, all 1
    integer wide;                          // a 32-bit variable

    initial begin
        $display("a + b + cin printed directly:        %0d", a + b + cin);
        wide = a + b + cin;
        $display("a + b + cin stored in an integer:    %0d", wide);
        $finish(0);
    end
endmodule
