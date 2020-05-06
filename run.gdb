exec-file open2x-bootloader.elf
target remote localhost:3333
make
load
add-symbol-file open2x-bootloader.elf
#cont
