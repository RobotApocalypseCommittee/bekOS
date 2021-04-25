kern_path=$(wslpath -am "$2")
"$1" -M raspi3 -serial stdio -d guest_errors,unimp -usb -device usb-kbd -device usb-mouse -kernel $kern_path