#!/bin/bash
#
# bekOS is a basic OS for the Raspberry Pi
# Copyright (C) 2024-2025 Bekos Contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

print_help()
{
  NAME=$(basename "$0")
  echo "Qemu runner for bekOS."
  echo
  echo "Usage: $NAME MACHINE EXECUTABLE [QEMU_CMD] [-d]"
  echo "  Supported MACHINEs: virt, rpi3."
  echo "  EXECUTABLE: The kernel image."
  echo "  QEMU_CMD: qemu command to run; if not set defaults to qemu-system-aarch64"
  each "  -d: Debug enabled. If set, GDB debug server will be enabled at port 1234 and machine paused."
}

usage ()
{
  print_help
  exit 1
}

[ -n "$1" ] || usage

QEMU_ARGS=()

case "$1" in
  virt)
    QEMU_ARGS+=(-machine virt -cpu cortex-a57 -usb -device qemu-xhci -device usb-kbd -device usb-mouse -device virtio-gpu-device -device "virtio-blk-device,drive=main_drive" -drive "format=raw,id=main_drive,file=fat:system,readonly=on,if=none" -serial mon:stdio -global virtio-mmio.force-legacy=false)
    ;;
  rpi3)
    QEMU_ARGS+=(-M raspi3b -serial null -serial mon:stdio -d "guest_errors,unimp" -usb -device usb-kbd -device usb-mouse)
    ;;
  *)
    echo "Unknown MACHINE: $1"
    exit 1
    ;;
esac

if [ "$3" = "-d" ] || [ "$4" = "-d" ]
then
  QEMU_ARGS+=(-s -S)
fi

if [ -n "$3" ] && [ "$3" != "-d" ]
then
  QEMU_CMD="$3"
else
  QEMU_CMD="qemu-system-aarch64"
fi

QEMU_ARGS+=("-kernel" "$2")

echo "Running $QEMU_CMD ${QEMU_ARGS[*]}"
"$QEMU_CMD" "${QEMU_ARGS[@]}"