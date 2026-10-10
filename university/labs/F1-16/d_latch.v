// A D latch: while en = 1 the output follows d ("transparent"); while en = 0 it holds.
`timescale 1ns/1ns
module d_latch (
    input  wire en,
    input  wire d,
    output reg  q
);
    always @(en or d)
        if (en)
            q = d;
endmodule
