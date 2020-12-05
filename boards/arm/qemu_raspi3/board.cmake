# Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)
set(QEMU_ARCH aarch64)
set(QEMU_KERNEL_OPTION "-kernel;${ZEPHYR_BINARY_DIR}/zephyr.bin")

set(QEMU_FLAGS_${ARCH}
  -M raspi3
  -smp 4
  -nographic
  )
board_set_debugger_ifnset(qemu)
