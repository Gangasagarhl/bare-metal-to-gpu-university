// Top level for the optional board lab: one decade digit on four LEDs, stepping
// once every DIV clock cycles. DIV must be set for YOUR board: (board clock
// frequency in hertz) x (seconds per step). The default only suits simulation.
// Untested on hardware in this build: no FPGA board was available.
`timescale 1ns/1ns
module board_top #(
    parameter DIV = 4                      // clock cycles per step (set for your board)
) (
    input  wire       clk,                 // the board's clock pin
    input  wire       btn_reset,           // a push button, 1 while pressed
    output reg  [3:0] led                  // four LEDs show the digit in binary
);
    reg [31:0] ticks;                      // counts clock cycles up to DIV - 1
    wire       step = (ticks == DIV - 1);  // 1 for one cycle every DIV cycles

    always @(posedge clk) begin
        if (btn_reset || step)
            ticks <= 32'd0;
        else
            ticks <= ticks + 32'd1;
    end

    always @(posedge clk) begin
        if (btn_reset)
            led <= 4'd0;
        else if (step)
            led <= (led == 4'd9) ? 4'd0 : led + 4'd1;
    end
endmodule
