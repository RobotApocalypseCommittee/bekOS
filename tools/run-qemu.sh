kern_path=$(wslpath -am "$2")
"$1" -M raspi3 -serial stdio -kernel $kern_path