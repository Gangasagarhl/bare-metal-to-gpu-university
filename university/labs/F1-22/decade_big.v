// Forensic evidence: Omar's decade counter. It passes the same simulation test as
// decade.v, but the FPGA tool's report surprised him.
`timescale 1ns/1ns
module decade_big (
    input  wire       clk,
    input  wire       reset,
    output wire [3:0] digit
);
    integer n;

    always @(posedge clk) begin
        if (reset || n == 9)
            n <= 0;
        else
            n <= n + 1;
    end

    assign digit = n[3:0];
endmodule
