// The same selector with every case assigned: y has a value for every input.
`timescale 1ns/1ns
module mux_fixed (
    input  wire [1:0] sel,
    input  wire       a,
    input  wire       b,
    input  wire       c,
    output reg        y
);
    always @(*) begin
        y = 1'b0;               // default first: sel = 3 gives 0
        if (sel == 2'd0)
            y = a;
        else if (sel == 2'd1)
            y = b;
        else if (sel == 2'd2)
            y = c;
    end
endmodule
