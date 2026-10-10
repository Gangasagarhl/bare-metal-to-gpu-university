// Course project reference testbench (HW201): the memory unit and the program counter
// together. 1) every address gets its own pattern and reads back; 2) an edge with we = 0
// changes nothing; 3) the PC addresses the RAM and 16 consecutive cycles fetch 16
// consecutive words; 4) a forward branch, a backward branch (negative offset) and a
// wrap-around branch; 5) priorities: reset over load over branch over inc.
`timescale 1ns/1ns
module project_tb;
    reg        clk = 0;
    reg        reset = 1, load = 0, branch = 0, inc = 0, we = 0;
    reg  [7:0] jump_to = 8'd0, offset = 8'd0, din = 8'd0, addr_w = 8'd0;
    wire [7:0] pc, dout;
    wire [7:0] addr = we ? addr_w : pc;          // the testbench writes; the PC fetches
    integer    i, errors = 0;

    pc_branch #(.WIDTH(8)) u_pc (.clk(clk), .reset(reset), .load(load), .branch(branch),
                                 .inc(inc), .jump_to(jump_to), .offset(offset), .pc(pc));
    ram256x8 u_ram (.clk(clk), .we(we), .addr(addr), .din(din), .dout(dout));

    always #5 clk = ~clk;

    task tick;                                   // one rising edge, then 1 unit for the flip-flops
        begin
            @(posedge clk); #1;
        end
    endtask

    task expect_pc(input [7:0] e, input [8*32:1] what);
        begin
            if (pc !== e) begin
                $display("  pc = %0d, expected %0d (%0s)  <-- wrong", pc, e, what);
                errors = errors + 1;
            end else
                $display("  pc = %3d  (%0s)", pc, what);
        end
    endtask

    initial begin
        // 1. fill the memory: word a holds the pattern a XOR 0x5A
        for (i = 0; i < 256; i = i + 1) begin
            we = 1; addr_w = i[7:0]; din = i[7:0] ^ 8'h5A;
            tick;
        end
        we = 0; reset = 0;
        // read back through the PC: reset, then load each address and read
        for (i = 0; i < 256; i = i + 1) begin
            load = 1; jump_to = i[7:0];
            tick;
            if (dout !== (i[7:0] ^ 8'h5A)) begin
                $display("  addr %0d: read %h, expected %h  <-- wrong", i, dout, i[7:0] ^ 8'h5A);
                errors = errors + 1;
            end
        end
        load = 0;
        $display("1. 256 words written and read back: %0d wrong", errors);
        // 2. an edge with we = 0 must not write
        we = 0; addr_w = 8'd7; din = 8'hFF; load = 1; jump_to = 8'd7;
        tick; load = 0;
        if (dout !== (8'd7 ^ 8'h5A)) begin $display("  we = 0 did not protect  <-- wrong"); errors = errors + 1; end
        $display("2. edge with we = 0: mem[7] = %h (expected %h)", dout, 8'd7 ^ 8'h5A);
        // 3. fetch 16 consecutive words from 100
        load = 1; jump_to = 8'd100; tick; load = 0; inc = 1;
        for (i = 0; i < 16; i = i + 1) begin
            if (pc !== 8'd100 + i[7:0] || dout !== ((8'd100 + i[7:0]) ^ 8'h5A)) begin
                $display("  fetch %0d: pc = %0d dout = %h  <-- wrong", i, pc, dout); errors = errors + 1;
            end
            tick;
        end
        inc = 0;
        $display("3. 16 consecutive fetches from 100: pc now %0d", pc);
        // 4. branches
        load = 1; jump_to = 8'd5; tick; load = 0; expect_pc(8'd5, "load 5");
        branch = 1; offset = 8'd10; tick; branch = 0; expect_pc(8'd15, "branch +10");
        branch = 1; offset = -8'd3; tick; branch = 0; expect_pc(8'd12, "branch -3 (offset 0xFD)");
        branch = 1; offset = -8'd13; tick; branch = 0; expect_pc(8'd255, "branch -13 wraps to 255");
        branch = 1; offset = 8'd6; tick; branch = 0; expect_pc(8'd5, "branch +6 wraps to 5");
        // 5. priorities
        branch = 1; inc = 1; offset = 8'd2; tick; branch = 0; inc = 0; expect_pc(8'd7, "branch wins over inc");
        load = 1; branch = 1; jump_to = 8'd200; tick; load = 0; branch = 0; expect_pc(8'd200, "load wins over branch");
        reset = 1; load = 1; branch = 1; inc = 1; tick; reset = 0; load = 0; branch = 0; inc = 0;
        expect_pc(8'd0, "reset wins over everything");
        inc = 1; tick; inc = 0; expect_pc(8'd1, "inc");
        tick; expect_pc(8'd1, "hold");
        if (errors == 0)
            $display("RESULT: PASS (memory unit and program counter: all checks)");
        else
            $display("RESULT: FAIL (%0d wrong checks)", errors);
        $finish(0);
    end
endmodule
