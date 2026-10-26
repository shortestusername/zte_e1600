/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * ZTE ZX279128 secondary CPU trampoline.
 *
 * The cspboot BootMonitor pen jumps straight to the address we put in
 * the IRAM mailbox, with the MMU and caches off but the L1 caches
 * still holding whatever cspboot left behind. The stock 4.1.25 kernel
 * handled this with a two-instruction stub (bl v7_invalidate_l1;
 * b secondary_startup); we do the same. Both branches are PC-relative
 * within .text, so they remain correct when executed at the physical
 * address the mailbox hands out.
 */

#include <linux/linkage.h>
#include <asm/assembler.h>

	.arm

ENTRY(zx279128_secondary_trampoline)
 ARM_BE8(setend	be)					@ ensure we are in BE8 mode
	bl	v7_invalidate_l1			@ discard cspboot L1 lines
	b	secondary_startup
ENDPROC(zx279128_secondary_trampoline)
