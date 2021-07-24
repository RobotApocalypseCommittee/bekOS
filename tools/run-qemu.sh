echo Starting Qemu
kern_path=$(wslpath -am "$2")
"$1" -M raspi3 -serial null -serial mon:stdio -d guest_errors,unimp -usb -device usb-kbd -device usb-mouse -kernel $kern_path