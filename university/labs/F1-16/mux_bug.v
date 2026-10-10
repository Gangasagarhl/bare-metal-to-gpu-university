// Forensic evidence: a 3-input selector written by Sam.
// Specification: y = a when sel = 0, b when sel = 1, c when sel = 2, and 0 when sel = 3.
`timescale 1ns/1ns
module mux_bug (
    input  wire [1:0] sel,
    input  wire       a,
    input  wire       b,
    input  wire       c,
    output reg        y
);
    always @(*) begin
        if (sel == 2'd0)
            y = a;
        else if (sel == 2'd1)
            y = b;
        else if (sel == 2'd2)
            y = c;
    end
endmodule
