// Testbench: how long until 4-, 8- and 16-bit ripple-carry adders settle?
// Case "long carry": a = all ones, b changes from 0 to 1 (the carry ripples through every bit).
// Case "no carry":   a = 0,        b changes from 0 to 1 (only bit 0 changes).
module tb_ripple_delay;
    reg [15:0] a, b;
    wire [3:0]  s4;
    wire [7:0]  s8;
    wire [15:0] s16;
    wire c4, c8, c16;
    integer last4, last8, last16, t0;

    adder_d #(4)  add4 (.a(a[3:0]), .b(b[3:0]), .cin(1'b0), .s(s4),  .cout(c4));
    adder_d #(8)  add8 (.a(a[7:0]), .b(b[7:0]), .cin(1'b0), .s(s8),  .cout(c8));
    adder_d #(16) add16(.a(a),      .b(b),      .cin(1'b0), .s(s16), .cout(c16));

    always @(s4 or c4)   last4  = $time;   // remember when each output last changed
    always @(s8 or c8)   last8  = $time;
    always @(s16 or c16) last16 = $time;

    task measure(input [15:0] new_a, input [255:0] label);
        begin
            a = new_a; b = 0;
            #200;                           // let everything settle first
            t0 = $time;
            b = 1;                          // the change we time
            #200;
            $display("%0s: settled after  4-bit %0d   8-bit %0d   16-bit %0d time units",
                     label, last4 - t0, last8 - t0, last16 - t0);
            $display("    results: 4-bit %b_%h  8-bit %b_%h  16-bit %b_%h",
                     c4, s4, c8, s8, c16, s16);
        end
    endtask

    initial begin
        measure(16'hFFFF, "long carry (all ones + 1)");
        measure(16'h0000, "no carry   (0 + 1)       ");
        $finish(0);
    end
endmodule
