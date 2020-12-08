/*
 * Copyright 2020 Broadcom
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/__assert.h>
#include <arch/arm/aarch64/irq.h>

extern void raspi3_c_helloworld(char *);

void z_soc_irq_init(void) {
	raspi3_c_helloworld(__func__);
}

void z_soc_irq_enable(unsigned int irq) {
	raspi3_c_helloworld(__func__);
}

void z_soc_irq_disable(unsigned int irq) {
	raspi3_c_helloworld(__func__);
}

int z_soc_irq_is_enabled(unsigned int irq) {
	raspi3_c_helloworld(__func__);
	return -1;
}

void z_soc_irq_priority_set(unsigned int irq, unsigned int prio, unsigned int flags) {
	raspi3_c_helloworld(__func__);
}

unsigned int z_soc_irq_get_active(void) {
	raspi3_c_helloworld(__func__);
	return 0;
}

void z_soc_irq_eoi(unsigned int irq) {
	raspi3_c_helloworld(__func__);
}
