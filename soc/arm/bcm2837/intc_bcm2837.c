/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/__assert.h>
#include <arch/arm/aarch64/irq.h>

/* TODO: Get these from device tree. */
#define PBASE 0x3f000000
#define BCM2836_ARMCTRL_BASE (PBASE+0x0000B200)

/* Definitions copied from linux's irq-bcm2835.c: */

/* Put the bank and irq (32 bits) into the hwirq */
#define MAKE_HWIRQ(b, n)	((b << 5) | (n))
#define HWIRQ_BANK(i)		(i >> 5)
#define HWIRQ_BIT(i)		BIT(i & 0x1f)

#define NR_IRQS_BANK0		8
#define BANK0_HWIRQ_MASK	0xff
/* Shortcuts can't be disabled so any unknown new ones need to be masked */
#define SHORTCUT1_MASK		0x00007c00
#define SHORTCUT2_MASK		0x001f8000
#define SHORTCUT_SHIFT		10
#define BANK1_HWIRQ		BIT(8)
#define BANK2_HWIRQ		BIT(9)
#define BANK0_VALID_MASK	(BANK0_HWIRQ_MASK | BANK1_HWIRQ | BANK2_HWIRQ \
					| SHORTCUT1_MASK | SHORTCUT2_MASK)

#define REG_FIQ_CONTROL		0x0c
#define FIQ_CONTROL_ENABLE	BIT(7)

#define NR_BANKS		3
#define IRQS_PER_BANK		32

static const int armctrl_reg_pending[] = { 0x04, 0x08, 0x00 };
static const int armctrl_reg_enable[] = { 0x10, 0x14, 0x18 };
static const int armctrl_reg_disable[] = { 0x1c, 0x20, 0x24 };
static const int armctrl_bank_bits[] = { 32, 32, 8 };

/* Copied from https://github.com/xinu-os/xinu/blob/master/system/platforms/arm-rpi/dispatch.c: */

/* Number of IRQs shared between the GPU and ARM. These correspond to the IRQs
 * that show up in the IRQ_pending_1 and IRQ_pending_2 registers.  */
#define ARMCTRL_NUM_GPU_SHARED_IRQS     64

/* Number of ARM-specific IRQs. These correspond to IRQs that show up in the
 * first 8 bits of IRQ_basic_pending.  */
#define ARMCTRL_NUM_ARM_SPECIFIC_IRQS   8

#define ARMCTRL_NUM_IRQS (ARMCTRL_NUM_GPU_SHARED_IRQS + ARMCTRL_NUM_ARM_SPECIFIC_IRQS)
#define L1_NUM_IRQS (32)
#define NUM_IRQS (ARMCTRL_NUM_IRQS + L1_NUM_IRQS)

static bool irq_enabled[NUM_IRQS];

static void armctrl_irq_to_bank(unsigned int irq, int *bank, int *bank_bit) {
	__ASSERT_NO_MSG(irq < ARMCTRL_NUM_IRQS);
	*bank = 0;
	*bank_bit = irq;
	while (*bank_bit >= armctrl_bank_bits[*bank]) {
		*bank_bit -= armctrl_bank_bits[*bank];
		*bank += 1;

		__ASSERT_NO_MSG(*bank <= 2);
	}
	__ASSERT_NO_MSG(0 <= *bank);
	__ASSERT_NO_MSG(*bank <= 2);
	__ASSERT_NO_MSG(0 <= *bank_bit);
	__ASSERT_NO_MSG(*bank_bit < 32);
}

static void armctrl_irq_enable(unsigned int irq) {
	int bank, bank_bit;
	armctrl_irq_to_bank(irq, &bank, &bank_bit);
	uintptr_t addr = BCM2836_ARMCTRL_BASE + armctrl_reg_enable[bank];

	printk("armctrl_irq_enable irq=%d, bank=%d, bit=%d, addr=%p\n", irq, bank, bank_bit, (char *) addr);
	sys_write32((1 << bank_bit), addr);
}

static void armctrl_irq_disable(unsigned int irq) {
	int bank, bank_bit;
	armctrl_irq_to_bank(irq, &bank, &bank_bit);
	uintptr_t addr = BCM2836_ARMCTRL_BASE + armctrl_reg_disable[bank];

	printk("armctrl_irq_enable irq=%d, bank=%d, bit=%d, addr=%p\n", irq, bank, bank_bit, (char *) addr);
	sys_write32((1 << bank_bit), addr);
}

/* BCM2836 ARM-local peripherals manual, section 4.10 "Core interrupt sources" */
/* QEMU Device Implementation: https://github.com/qemu/qemu/blob/918c81a53eb18ec4e9979876a39e86610cc565f4/hw/intc/bcm2836_control.c */

static void l1_irq_enable(unsigned int l1_irq) {
	__ASSERT_NO_MSG(l1_irq < L1_NUM_IRQS);

	if (0 <= l1_irq && l1_irq <= 3) {
		/* Core timer interrupts: CNT_PS_IRQ = 0, CNT_PNS_IRQ = 1, CNT_HP_IRQ = 2, CNT_V_IRQ = 3 */
		int cpuid = arch_curr_cpu()->id;
		intptr_t core_timer_interrupt_control_addr = 0x40000040 + cpuid * 4;
		uint32_t val = sys_read32(core_timer_interrupt_control_addr);
		val |= (1 << l1_irq);
		sys_write32(val, core_timer_interrupt_control_addr);
	} else {
		printk("Warning: l1_irq_enable(%d) not implemented.\n", l1_irq);
	}
}

static void l1_irq_disable(unsigned int l1_irq) {
	__ASSERT_NO_MSG(l1_irq < L1_NUM_IRQS);
	printk("Warning: l1_irq_disable(%d) not implemented.\n", l1_irq);
}

void z_soc_irq_init(void) {
	/* Don't print anything here as C might not be initialized yet. */
}

int z_soc_irq_is_enabled(unsigned int irq) {
	/* Manual does not explicitly state that reading from armctrl_reg_enable
	 * indicates if an IRQ was enabled, also Xinu does not do it like
	 * this. That's why we have irq_enabled. */
	__ASSERT_NO_MSG(irq < ARMCTRL_NUM_IRQS);
	return irq_enabled[irq];
}

void z_soc_irq_enable(unsigned int irq) {
	if (irq < ARMCTRL_NUM_IRQS) {
		armctrl_irq_enable(irq);
		irq_enabled[irq] = true;
	} else {
		l1_irq_enable(irq - ARMCTRL_NUM_IRQS);
	}
}

void z_soc_irq_disable(unsigned int irq) {
	if (irq < ARMCTRL_NUM_IRQS) {
		armctrl_irq_disable(irq);
		irq_enabled[irq] = false;
	} else {
		l1_irq_disable(irq - ARMCTRL_NUM_IRQS);
	}
}

void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, unsigned int flags) {
	/* Not supported. */

	/* Peripheral Manual: There is no priority for any interrupt. If one
	 * interrupt is much more important then all others it canbe routed to
	 * the FIQ. */
	return;
}

/* Called from HW IRQ context with interrupts disabled: Retrieve the currently
 * active IRQ that should be handled.
 *
 * Resources:
 * - https://embedded-xinu.readthedocs.io/en/latest/arm/rpi/BCM2835-Interrupt-Controller.html
 */
unsigned int z_soc_irq_get_active(void) {
	int cpuid = arch_curr_cpu()->id;

	/* printk("%s on cpu%d, stack at ~%p\n", __func__, cpuid, &cpuid); */

	for (unsigned int irq = 0; irq < ARMCTRL_NUM_IRQS; irq++) {
		int bank, bank_bit;
		armctrl_irq_to_bank(irq, &bank, &bank_bit);

		unsigned int bank_pending = sys_read32(BCM2836_ARMCTRL_BASE + armctrl_reg_pending[bank]);
		bool is_pending = (bank_pending & (1 << bank_bit)) != 0;

		if (is_pending) {
			/* Clear pending to allow us to detect when this IRQ occurs again (nested irq). */
			sys_write32(bank_pending & ~(1 << bank_bit), BCM2836_ARMCTRL_BASE + armctrl_reg_pending[bank]);
			return irq;
		}
	}

	intptr_t core_interrupt_sources_addr = 0x40000060 + cpuid * 4;
	uint32_t pending = sys_read32(core_interrupt_sources_addr);
	for (unsigned int l1_irq = 0; l1_irq < L1_NUM_IRQS; l1_irq++) {
		unsigned int irq = ARMCTRL_NUM_IRQS + l1_irq;
		if (pending & (1 << l1_irq)) {
			/* https://developer.arm.com/architectures/learn-the-architecture/generic-timer/single-page */
			if (l1_irq == 3) {
				arm_arch_timer_set_irq_mask(true);
			}
			return irq;
		}
	}

	__ASSERT_NO_MSG(false);
	CODE_UNREACHABLE;
}

/* Mark irq as inactive (called in HW IRQ context after handler). */
void z_soc_irq_eoi(unsigned int irq) {
	if (irq < ARMCTRL_NUM_IRQS) {
		printk("Warning: %s(%d)\n", __func__, irq);
	} else {
		unsigned int l1_irq = irq - ARMCTRL_NUM_IRQS;
		if (l1_irq == 3) {
			arm_arch_timer_set_irq_mask(false);
		}
	}
}
