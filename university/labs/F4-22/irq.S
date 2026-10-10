/* irq.S - F4-22 interrupt entry stubs (32-bit). irq0_stub saves the general registers, calls
 * the C++ timer handler and returns with iret; fault_stub reports any other vector and stops. */
        .section .text
        .globl irq0_stub
irq0_stub:
        pusha
        cld
        call irq0_handler
        popa
        iret

        .globl fault_stub
fault_stub:
        cli
        call fault_handler                 /* does not return */
1:      hlt
        jmp 1b

        .section .note.GNU-stack, "", @progbits
