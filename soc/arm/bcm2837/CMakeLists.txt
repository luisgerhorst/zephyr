# Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

zephyr_library_sources_ifdef(CONFIG_ARM_MMU mmu_regions.c)

zephyr_sources_ifdef(CONFIG_SOC_BCM2837    plat_core.S)
zephyr_sources_ifdef(CONFIG_SOC_BCM2837    intc_bcm2837.c)
