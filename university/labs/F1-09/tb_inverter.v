// Testbench: try every input of the inverter and compare with the expected output.
module tb_inverter;
    reg a;
    wire y;
    integer i;
    integer errors = 0;

    inverter dut(.a(a), .y(y));

    initial begin
        $display("a | y | expected");
        for (i = 0; i < 2; i = i + 1) begin
            a = i[0];
            #1;                                   // let the switches settle
            $display("%b | %b | %b", a, y, ~a);
            if (y !== ~a) errors = errors + 1;    // !== also catches x and z
        end
        if (errors == 0) $display("ALL PASS (2 vectors)");
        else $display("FAIL: %0d of 2 vectors wrong", errors);
        $finish(0);
    end
endmodule
