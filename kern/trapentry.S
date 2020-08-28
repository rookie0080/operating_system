/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>



###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);		/* CPU 已经自动压入了一个error code */					\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

TRAPHANDLER_NOEC(handler0, T_DIVIDE)
TRAPHANDLER_NOEC(handler1, T_DEBUG)
TRAPHANDLER_NOEC(handler2, T_NMI)
TRAPHANDLER_NOEC(handler3, T_BRKPT)
TRAPHANDLER_NOEC(handler4, T_OFLOW)
TRAPHANDLER_NOEC(handler5, T_BOUND)
TRAPHANDLER_NOEC(handler6, T_ILLOP)
TRAPHANDLER_NOEC(handler7, T_DEVICE)

TRAPHANDLER(handler8, T_DBLFLT)
TRAPHANDLER(handler10, T_TSS)
TRAPHANDLER(handler11, T_SEGNP)
TRAPHANDLER(handler12, T_STACK)
TRAPHANDLER(handler13, T_GPFLT)
TRAPHANDLER(handler14, T_PGFLT)
TRAPHANDLER(handler17, T_ALIGN)

TRAPHANDLER_NOEC(handler16, T_FPERR)											
TRAPHANDLER_NOEC(handler18, T_MCHK)
TRAPHANDLER_NOEC(handler19, T_SIMDERR)

// 其他不重要的中段号，只是为了后面填充idt
TRAPHANDLER(handler9, 9)
TRAPHANDLER(handler15, 15)

/*
 * Lab 3: Your code here for _alltraps
 */

_alltraps: /* 可以模仿env.c中env_pop_tf函数，取逆过程 */
// 	/* push values to make the stack look like a struct Trapframe */
	pushl %ds
	pushl %es
	pushal
	// load GD_KD into %ds and %es
	movw $(GD_KD), %ax	// 16位
	movw %ax, %ds
	movw %ax, %es
	/ *pushl %esp to pass a pointer to the Trapframe as an argument to trap() */
	pushl %esp		// esp此时指向栈顶，指向之前创建的Trapframe
	call trap

.data
.globl handlers
handlers:
	.long handler0
	.long handler1
	.long handler2
	.long handler3
	.long handler4
	.long handler5
	.long handler6
	.long handler7
	.long handler8
	.long handler9
	.long handler10
	.long handler11
	.long handler12
	.long handler13
	.long handler14
	.long handler15
	.long handler16
	.long handler17
	.long handler18
	.long handler19
