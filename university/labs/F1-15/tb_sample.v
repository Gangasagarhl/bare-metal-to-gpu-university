// Testbench: an 8-bit ripple-carry adder whose result is read WAIT time units
// after the inputs change, the way a later circuit with a fixed deadline would read it.
module tb_sample;
    parameter WAIT = 8;
    reg [7:0] a, b;
    wire [7:0] s;
    wire cout;
    integer i, expected, wrong;
    reg [15:0] vec [0:7];

    adder_d #(8) dut(.a(a), .b(b), .cin(1'b0), .s(s), .cout(cout));

    initial begin
        vec[0] = {8'd2,   8'd3};   vec[1] = {8'd85,  8'd170};
        vec[2] = {8'd100, 8'd27};  vec[3] = {8'd3,   8'd1};
        vec[4] = {8'd15,  8'd1};   vec[5] = {8'd127, 8'd1};
        vec[6] = {8'd255, 8'd1};   vec[7] = {8'd64,  8'd64};
        a = 0; b = 0; wrong = 0;
        #100;
        $display("result read %0d time units after the inputs change", WAIT);
        $display("  a  +   b  | expected | read | check");
        for (i = 0; i < 8; i = i + 1) begin
            {a, b} = vec[i];
            #(WAIT);
            expected = a + b;
            $display("%3d + %3d  |   %3d    | %3d  | %s", a, b, expected, {cout, s},
                     ({cout, s} === expected[8:0]) ? "ok" : "WRONG");
            if ({cout, s} !== expected[8:0]) wrong = wrong + 1;
            #100;                              // back to rest before the next vector
            a = 0; b = 0;
            #100;
        end
        if (wrong == 0) $display("ALL PASS (8 vectors)");
        else $display("FAIL: %0d of 8 vectors read wrong", wrong);
        $finish(0);
    end
endmodule
