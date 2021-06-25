/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2015 Jonas Norling <jonas.norling@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * ABI: r0..r3 are caller-saved (scratch registers), R4..R12 are callee-saved.
 * It is appropriate to use R12 for a system call opcode (saved by NVIC). The
 * stack pointer points to the current extent of the stack -- it is decremented
 * before being used as index in a store. The stack grows downwards, to lower
 * addresses. When an interrupt is processed, 8 registers are stored. LR is set
 * to a special value that makes an ordinary function return into a return from
 * interrupt. The LR value indicates which stack is going to be used (process
 * or main) and can be modified before return.
 *
 *                  ____________________
 *           Stack |                    |
 *                 |                    |
 *    higher       |        R4          | <-- SP saved in TCB (64B context)
 *  addresses      |        R5          |   ^
 *      |  ^       |        R6          |   |
 *      |  |       |        R7          |   | 8 registers pushed by handler:
 *      |  |       |        R8          |   | R4..R11
 *      |  |       |        R9          |   | Full task context is now stored
 *      V  |       |        R10         |   |
 *         |       |        R11         |   |
 *     direction   |        R0          | <-- SP when SVC handler gets control
 *     of growth   |        R1          |   ^
 *                 |        R2          |   |
 *                 |        R3          |   | 8 registers are pushed by
 *                 |        R12         |   | the NVIC hardware:
 *                 |        LR (R14)    |   | xPSR, PC, LR, R12, R3..R0
 *                 |        PC (R15)    |   |
 *                 |       xPSR         |   |
 *                 |                    | <-- SP before SVC
 *                 |      (stuff)       |
 *       Stack +   |                    |
 *       StackSize |____________________|
 *
 */

#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>

#include "scheduler.h"

/* Current state of a task */
struct task_control_block {
	/* Current stack pointer when switched out */
	void *sp;
	/* Task is runnable, i.e. not blocked waiting for something */
	bool runnable;
};

struct stack_frame {
	/* Registers that the software has to push on the stack, after the
	 * NVIC has pushed some specific registers.
	 * Corresponds to registers pushed by "STMDB %0!, {r4-r11}". */
	struct {
		uint32_t registers[8];
	} software_frame;

	/* Registers pushed on the stack by NVIC (hardware), before the other
	 * registers defined above. */
	struct {
		uint32_t r0;
		uint32_t r1;
		uint32_t r2;
		uint32_t r3;
		uint32_t r12;
		void *lr;
		void *pc;
		uint32_t psr;
	} nvic_frame;
};


static struct task_control_block tcbs[SCHEDULER_NUM_TASKS];
static uint8_t current_task;
static const uint8_t STACK_FILL = 0xa5;


static void return_from_task(void)
{
	while (true);
}

void scheduler_init(void)
{
	int i;
	for (i = 0; i < SCHEDULER_NUM_TASKS; i++) {
		const struct task_data *task = &tasks[i];

		/* Set stack pointer to beginning of stack */
		tcbs[i].sp = (uint8_t *)task->stack + task->stack_size;

		/* Fill stack with a canary value to ease debugging. */
		memset(task->stack, STACK_FILL, task->stack_size);

		/* Push the same thing that a PendSV would push on the task's
		 * stack, with dummy values for the general purpose registers.
		 */
		tcbs[i].sp -= sizeof(struct stack_frame);
		struct stack_frame *frame = (struct stack_frame *)tcbs[i].sp;
		frame->nvic_frame.r0 = 0xff00ff00;
		frame->nvic_frame.r1 = 0xff00ff01;
		frame->nvic_frame.r2 = 0xff00ff02;
		frame->nvic_frame.r3 = 0xff00ff03;
		frame->nvic_frame.r12 = 0xff00ff0c;
		frame->nvic_frame.lr = return_from_task;
		frame->nvic_frame.pc = task->entry_point;
		frame->nvic_frame.psr = 0x21000000; /* Default, allegedly */
		frame->software_frame.registers[0] = 0xff00ff04;
		frame->software_frame.registers[1] = 0xff00ff05;
		frame->software_frame.registers[2] = 0xff00ff06;
		frame->software_frame.registers[3] = 0xff00ff07;
		frame->software_frame.registers[4] = 0xff00ff08;
		frame->software_frame.registers[5] = 0xff00ff09;
		frame->software_frame.registers[6] = 0xff00ff0a;
		frame->software_frame.registers[7] = 0xff00ff0b;

		tcbs[i].runnable = true;
	}
}

/*
 * Logic for doing a task switch. This implementation simply switches to the
 * next available task in the list.
 */
static void scheduler_switch(void)
{
	assert(tcbs[current_task].sp >= tasks[current_task].stack);
	assert(((uint8_t *)tasks[current_task].stack)[0] == STACK_FILL);

	/* Find the next runnable task in a round-robin fashion. There must
	 * always be a runnable task, such as an idle task. */
	do {
		current_task = (current_task + 1) % SCHEDULER_NUM_TASKS;
	} while (!tcbs[current_task].runnable);
}

/**
 * This PendSV interrupt is triggered by a task doing a system call.
 *
 * The interrupt can come from a task that uses the main or program stack
 * pointer (MSP or PSP). In the latter case we are free to do whatever we want
 * here, because the task's stack won't be affected (ISRs run on the main
 * stack). In the former case, we need to take some care with which registers
 * (R4..R11) the compiler pushes for its own use.
 */
void __attribute__((naked)) pend_sv_handler(void)
{
	const uint32_t RETURN_ON_PSP = 0xfffffffd;

	/* 0. NVIC has already pushed some registers on the program/main stack.
	 * We are free to modify R0..R3 and R12 without saving them again, and
	 * additionally the compiler may choose to use R4..R11 in this function.
	 * If it does so, the naked attribute will prevent it from saving those
	 * registers on the stack, so we'll just have to hope that it doesn't do
	 * anything with them before our stm or after our ldm instructions.
	 * Luckily, we don't ever intend to return to the original caller on the
	 * main stack, so this question is moot. */

	/* Read the link register */
	uint32_t lr;
	__asm__("MOV %0, lr" : "=r" (lr));

	if (lr & 0x4) {
		/* This PendSV call was made from a task using the PSP */

		/* 1. Push all other registers (R4..R11) on the program stack */
		void *psp;
		__asm__(
			/* Load PSP to a temporary register */
			"MRS %0, psp\n"
			/* Push context relative to the address in the temporary
			 * register, update register with resulting address */
			"STMDB %0!, {r4-r11}\n"
			/* Put back the new stack pointer in PSP (pointless) */
			"MSR psp, %0\n"
			: "=r" (psp));

		/* 2. Store that PSP in the current TCB */
		tcbs[current_task].sp = psp;
	} else {
		/* This PendSV call was made from a task using the MSP. This
		 * code is not equipped to return to the main task, but we store
		 * the proper registers here anyway for good form. */

		/* 1. Push all other registers (R4..R11) on the main stack */
		__asm__(
			/* Push context on main stack */
			"STMDB SP!, {r4-r11}");
	}

	/* 3. Call context switch function, changes current TCB */
	scheduler_switch();

	/* 4. Load PSP from TCB */
	/* 5. Pop R4..R11 from the program stack */
	void *psp = tcbs[current_task].sp;
	__asm__(
		/* Pop context relative temporary register, update register */
		"LDMFD %0!, {r4-r11}\n"
		/* Put back the stack pointer in PSP */
		"MSR psp, %0\n"
		: : "r" (psp));

	/* 6. Return. NVIC will pop registers and find the PC to use there. */
	__asm__("bx %0" : : "r"(RETURN_ON_PSP));
}

void scheduler_yield(void)
{
	/* Trigger PendSV, causing pend_sv_handler to be called immediately */
	SCB_ICSR |= SCB_ICSR_PENDSVSET;
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
	__asm__("nop");
}
