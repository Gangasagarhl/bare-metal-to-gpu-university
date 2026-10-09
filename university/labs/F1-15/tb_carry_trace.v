// Testbench: watch the carries of a 4-bit ripple-carry adder change, one by one.
// a = 1111 stays fixed; b changes from 0000 to 0001 at time 100.
module tb_carry_trace;
    reg [3:0] a, b;
    wire [3:0] s;
    wire cout;

    adder_d #(4) dut(.a(a), .b(b), .cin(1'b0), .s(s), .cout(cout));

    initial begin
        a = 4'b1111; b = 4'b0000;
        #100;
        $display("time | carries c4 c3 c2 c1 | sum s3..s0");
        $monitor("%4t |          %b  %b  %b  %b |   %b", $time,
                 dut.c[4], dut.c[3], dut.c[2], dut.c[1], s);
        b = 4'b0001;                       // the change, at time 100
        #50;
        $finish(0);
    end
endmodule
