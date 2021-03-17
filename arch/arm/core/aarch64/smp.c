/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/**
 * @file
 * @brief codes required for AArch64 multicore and Zephyr smp support
 */

#include <cache.h>
#include <device.h>
#include <kernel.h>
#include <kernel_structs.h>
#include <ksched.h>
#include <soc.h>
#include <init.h>
#include <arch/arm/aarch64/arm_mmu.h>
#include <arch/cpu.h>
#include <drivers/interrupt_controller/gic.h>
#include <drivers/pm_cpu_ops.h>
#include <sys/arch_interface.h>

#define SGI_SCHED_IPI	0

volatile struct {
	void *sp; /* Fixed at the first entry */
	arch_cpustart_t fn;
	void *arg;
	char pad[] __aligned(L1_CACHE_BYTES);
} arm64_cpu_init[CONFIG_MP_NUM_CPUS];

/*
 * _curr_cpu is used to record the struct of _cpu_t of each cpu.
 * for efficient usage in assembly
 */
volatile _cpu_t *_curr_cpu[CONFIG_MP_NUM_CPUS];

#ifdef CONFIG_SOC_BCM2837
static ALWAYS_INLINE void __SEV(void)
{
	__asm__ volatile ("sev" : : : "memory");
}

static void sys_write64(uint64_t val, uintptr_t addr) {
	sys_write32(val, addr);
	sys_write32(val >> 32, addr + sizeof(uint32_t));
}
#endif

extern void __start(void);
/* Called from Zephyr initialization */
void arch_start_cpu(int cpu_num, k_thread_stack_t *stack, int sz,
		    arch_cpustart_t fn, void *arg)
{
	__ASSERT(sizeof(arm64_cpu_init[0]) == ARM64_CPU_INIT_SIZE,
		 "ARM64_CPU_INIT_SIZE != sizeof(arm64_cpu_init[0]\n");

	_curr_cpu[cpu_num] = &(_kernel.cpus[cpu_num]);
	arm64_cpu_init[cpu_num].fn = fn;
	arm64_cpu_init[cpu_num].arg = arg;
	arm64_cpu_init[cpu_num].sp =
		(void *)(Z_THREAD_STACK_BUFFER(stack) + sz);

	arch_dcache_range((void *)&arm64_cpu_init[cpu_num],
			  sizeof(arm64_cpu_init[cpu_num]), K_CACHE_WB_INVD);

#ifdef CONFIG_SOC_BCM2837
	sys_write64((uint64_t) &__start, 0xd8 + 0x8 * cpu_num);
	__DSB();
	__SEV();
#else
	/* TODO: get mpidr from device tree, using cpu_num */
	if (pm_cpu_on(cpu_num, (uint64_t)&__start))
		printk("Failed to boot CPU%d\n", cpu_num);
#endif

	/* Wait secondary cores up, see z_arm64_secondary_start */
	while (arm64_cpu_init[cpu_num].fn) {
		wfe();
	}
}

/* the C entry of secondary cores */
void z_arm64_secondary_start(void)
{
	arch_cpustart_t fn;
	int cpu_num = MPIDR_TO_CORE(GET_MPIDR());

	z_arm64_mmu_init();

#ifdef CONFIG_SMP
#ifndef CONFIG_SOC_BCM2837
	arm_gic_secondary_init();

	irq_enable(SGI_SCHED_IPI);
#endif
#endif

	fn = arm64_cpu_init[cpu_num].fn;

	/*
	 * Secondary core clears .fn to announce its presence.
	 * Primary core is polling for this.
	 */
	arm64_cpu_init[cpu_num].fn = NULL;

	dsb();
	sev();

	fn(arm64_cpu_init[cpu_num].arg);
}

#ifdef CONFIG_SMP
#ifndef CONFIG_SOC_BCM2837
void sched_ipi_handler(const void *unused)
{
	ARG_UNUSED(unused);

	z_sched_ipi();
}

/* arch implementation of sched_ipi */
void arch_sched_ipi(void)
{
	const uint64_t mpidr = GET_MPIDR();
	/*
	 * Send SGI to all cores except itself
	 * Note: Assume only one Cluster now.
	 */
	gic_raise_sgi(SGI_SCHED_IPI, mpidr, SGIR_TGT_MASK & ~(1 << MPIDR_TO_CORE(mpidr)));
}

static int arm64_smp_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* necessary master core init */
	_curr_cpu[0] = &(_kernel.cpus[0]);

	/*
	 * SGI0 is use for sched ipi, this might be changed to use Kconfig
	 * option
	 */
	IRQ_CONNECT(SGI_SCHED_IPI, IRQ_DEFAULT_PRIORITY, sched_ipi_handler, NULL, 0);

	irq_enable(SGI_SCHED_IPI);

	return 0;
}
SYS_INIT(arm64_smp_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
#endif
#endif
