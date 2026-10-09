// An SR latch made of two cross-coupled NOR gates.
// Each gate has a delay of 1 time unit, so the simulator shows the order of events.
`timescale 1ns/1ns
module sr_latch (
    input  wire s,      // set: make q = 1
    input  wire r,      // reset: make q = 0
    output wire q,
    output wire qn      // normally the opposite of q
);
    nor #1 g1 (q, r, qn);   // q  = NOT (r OR qn)
    nor #1 g2 (qn, s, q);   // qn = NOT (s OR q)
endmodule
