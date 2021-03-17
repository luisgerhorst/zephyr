# Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

set(EMU_PLATFORM qemu)
set(QEMU_ARCH aarch64)
set(QEMU_KERNEL_OPTION "-kernel;${ZEPHYR_BINARY_DIR}/zephyr.bin")

# https://github.com/s-matyukevich/raspberry-pi-os/issues/8#issuecomment-411024746
set(QEMU_FLAGS_${ARCH}
  -M raspi3
  -smp 4
  -nographic
  -no-reboot
  # To see Mini UART on qemu's stdout (default is PL011):
  # -serial null -serial chardev:con -monitor none
  )
board_set_debugger_ifnset(qemu)
