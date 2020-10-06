# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  cpu_idle.c
  cpu_smp.c
  fatal.c
  irq_manage.c
  isr.S
  prep_c.c
  reset.S
  swap.S
  thread.c
)

zephyr_library_sources_ifdef(CONFIG_IRQ_OFFLOAD irq_offload.c)
zephyr_library_sources_ifdef(CONFIG_THREAD_LOCAL_STORAGE tls.c)
