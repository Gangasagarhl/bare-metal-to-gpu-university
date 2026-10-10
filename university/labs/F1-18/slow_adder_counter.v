// A counter whose "+1" logic is slow: the adder's result appears 7 time units after
// its input changes (an exercise value, not the delay of any real part).
// The flip-flops copy next into count at every rising edge of clk.
`timescale 1ns/1ns
module slow_adder_counter (
    input  wire       clk,
    input  wire       reset,
    output reg  [3:0] count
);
    wire [3:0] next;
    assign #7 next = count + 4'd1;     // the slow combinational path

    always @(posedge clk) begin
        if (reset)
            count <= 4'd0;
        else
            count <= next;
    end
endmodule
