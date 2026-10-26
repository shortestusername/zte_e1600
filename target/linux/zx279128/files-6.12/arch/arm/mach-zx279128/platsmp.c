// SPDX-License-Identifier: GPL-2.0-only
/*
 * ZTE ZX279128 (ZXHN E1600) secondary CPU bring-up.
 *
 * The dual Cortex-A9 cores are already powered and clocked at kernel
 * entry: U-Boot/cspboot leaves a small "BootMonitor" pen in IRAM at
 * 0x00200800 and CPU1 idles there in a wfe loop:
 *
 *   0x00200858: wfe
 *   0x0020085c: ldr r0, [pc, #0x140]   ; reads the word at 0x002009a4
 *   0x00200860: mov pc, r0
 *
 * The mailbox word at 0x002009a4 holds 0x00200800 (self-loop) until the
 * primary writes a full 32-bit absolute startup address into it and
 * issues sev(); CPU1 then branches straight to that address with MMU
 * and caches off. This was verified by dumping live IRAM under our own
 * kernel (Gate F research, 2026-10-20) and by disassembling the stock
 * 4.1.25 kernel's prepare_cpus/boot_secondary, which use exactly this
 * protocol (plus scu_enable() on the primary and
 * scu_power_mode(SCU_PM_NORMAL) on the secondary). No PCU/power-on
 * register writes are involved and no code copy is needed because the
 * jump is a full absolute address: the mailbox points straight at our
 * in-kernel trampoline (headsmp.S), which invalidates the stale L1
 * caches and branches to secondary_startup, exactly like the stock
 * kernel's two-instruction stub.
 */

#include <linux/init.h>
#include <linux/io.h>
#include <linux/printk.h>
#include <linux/sizes.h>

#include <asm/barrier.h>
#include <asm/cacheflush.h>
#include <asm/smp.h>
#include <asm/smp_plat.h>
#include <asm/smp_scu.h>

/* Physical base of the cspboot BootMonitor image in IRAM. */
#define ZX_IRAM_BASE		0x00200800
/* Offset of the CPU1 startup-address mailbox (0x002009a4). */
#define ZX_MBOX_OFFSET		0x1a4

/* headsmp.S: bl v7_invalidate_l1; b secondary_startup (stock stub). */
void zx279128_secondary_trampoline(void);

static void __iomem *zx_scu_base;

/*
 * Enable the SCU (cache coherency between the two cores). The mailbox
 * is deliberately NOT armed here: the pen keeps self-looping until
 * boot_secondary() points it at the kernel, which only happens once
 * __cpu_up() has published and cache-cleaned secondary_data.
 */
static void zx279128_prepare_cpus(unsigned int max_cpus)
{
	void __iomem *scu_base;

	scu_base = ioremap(scu_a9_get_base(), SZ_256);
	if (!scu_base) {
		pr_err("zx279128: failed to map SCU, SMP disabled\n");
		return;
	}
	scu_enable(scu_base);
	zx_scu_base = scu_base;
}

/*
 * __cpu_up() has already filled secondary_data and written back the
 * dirty lines (sync_cache_w), but the idmap page tables and any other
 * data CPU1 reads on its way in may still sit dirty in this core's
 * caches, and CPU1 arrives with its MMU and caches off. Clean the
 * caches out to memory before waking it (same pattern as
 * rockchip/socfpga/shmobile platsmp), then point the BootMonitor
 * mailbox at secondary_startup and send the event.
 */
static int zx279128_boot_secondary(unsigned int cpu, struct task_struct *idle)
{
	void __iomem *mbox;

	mbox = ioremap(ZX_IRAM_BASE, SZ_4K);
	if (!mbox)
		return -ENODEV;

	flush_cache_all();
	smp_wmb();
	writel(__pa_symbol(zx279128_secondary_trampoline),
	       mbox + ZX_MBOX_OFFSET);
	dsb_sev();
	iounmap(mbox);

	return 0;
}

/* Take this core out of WFI power mode in the SCU (stock behaviour). */
static void zx279128_secondary_init(unsigned int cpu)
{
	if (zx_scu_base)
		scu_power_mode(zx_scu_base, SCU_PM_NORMAL);
}

static const struct smp_operations zx279128_smp_ops __initconst = {
	.smp_prepare_cpus	= zx279128_prepare_cpus,
	.smp_secondary_init	= zx279128_secondary_init,
	.smp_boot_secondary	= zx279128_boot_secondary,
};

CPU_METHOD_OF_DECLARE(zx279128_smp, "zte,zx279128-smp", &zx279128_smp_ops);
