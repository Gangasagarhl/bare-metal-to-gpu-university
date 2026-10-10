// Write 1, hold, read, write 0, hold, read: watch the cell keep its bit while wl = 0.
`timescale 1ns/1ns
module sram_cell_tb;
    reg  wl = 0;
    reg  drive = 0;          // 1 while the write drivers are switched on
    reg  value = 0;          // the bit to write
    wire bl;
    wire bln;
    integer errors = 0;

    // strong write drivers on the bit lines (high impedance when drive = 0)
    bufif1 (strong0, strong1) wbl  (bl,  value,  drive);
    bufif1 (strong0, strong1) wbln (bln, ~value, drive);

    sram_cell bitcell (.wl(wl), .bl(bl), .bln(bln));

    task show(input [8*12-1:0] what);
        $display("t=%3t %s wl=%b  q=%b qn=%b  bl=%b", $time, what, wl, bitcell.q, bitcell.qn, bl);
    endtask

    initial begin
        #5  show("start");
        // write 1
        value = 1; drive = 1; wl = 1; #5 show("write 1");
        wl = 0; drive = 0;            #20 show("hold");
        // read: connect the cell to the undriven bit line and look at it
        wl = 1;                        #5 show("read");
        if (bl !== 1'b1) errors = errors + 1;
        wl = 0;                        #5;
        // write 0
        value = 0; drive = 1; wl = 1; #5 show("write 0");
        wl = 0; drive = 0;            #20 show("hold");
        wl = 1;                        #5 show("read");
        if (bl !== 1'b0) errors = errors + 1;
        wl = 0;
        if (errors == 0)
            $display("RESULT: PASS (the cell held and returned both values)");
        else
            $display("RESULT: FAIL (%0d wrong reads)", errors);
        $finish(0);
    end
endmodule
