// Slower rhythms from one clock.
// div2 and div4 are the two lowest bits of a counter: they toggle at half and a
// quarter of the clock's rate. tick is a one-cycle pulse every 5 clock cycles: the
// rest of the design keeps using clk and does its slow work only when tick = 1.
`timescale 1ns/1ns
module divider (
    input  wire clk,
    input  wire reset,
    output wire div2,
    output wire div4,
    output reg  tick
);
    reg [1:0] bits;
    reg [2:0] five;      // counts 0, 1, 2, 3, 4, 0, ...

    assign div2 = bits[0];
    assign div4 = bits[1];

    always @(posedge clk) begin
        if (reset) begin
            bits <= 2'd0;
            five <= 3'd0;
            tick <= 1'b0;
        end else begin
            bits <= bits + 2'd1;
            five <= (five == 3'd4) ? 3'd0 : five + 3'd1;
            tick <= (five == 3'd4);
        end
    end
endmodule
