// Testbench: a short list of named test vectors, then all 512 input combinations.
module tb_rca4;
    reg [3:0] a, b;
    reg cin;
    wire [3:0] s;
    wire cout;
    integer i, expected;
    integer errors = 0;
    reg [8:0] vec [0:7];                   // {cin, a, b} for the named vectors

    rca4 dut(.a(a), .b(b), .cin(cin), .s(s), .cout(cout));

    initial begin
        vec[0] = {1'b0, 4'd1, 4'd1};   vec[1] = {1'b0, 4'd3, 4'd1};
        vec[2] = {1'b0, 4'd7, 4'd1};   vec[3] = {1'b0, 4'd4, 4'd4};
        vec[4] = {1'b0, 4'd4, 4'd0};   vec[5] = {1'b0, 4'd9, 4'd6};
        vec[6] = {1'b0, 4'd15, 4'd1};  vec[7] = {1'b1, 4'd5, 4'd2};
        $display(" a + b + cin | expected | got (cout s) | check");
        for (i = 0; i < 8; i = i + 1) begin
            {cin, a, b} = vec[i];
            #1;
            expected = a + b + cin;
            $display("%2d + %2d + %0d  |    %2d    |   %2d (%b %b) | %s", a, b, cin,
                     expected, {cout, s}, cout, s, ({cout, s} === expected[4:0]) ? "ok" : "WRONG");
        end
        for (i = 0; i < 512; i = i + 1) begin
            {cin, a, b} = i[8:0];
            #1;
            expected = a + b + cin;
            if ({cout, s} !== expected[4:0]) errors = errors + 1;
        end
        if (errors == 0) $display("ALL PASS (512 of 512 input combinations)");
        else $display("FAIL: %0d of 512 input combinations wrong", errors);
        $finish(0);
    end
endmodule
