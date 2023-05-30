python3 ./build.py
../bin/compiler ../aaa.sy -S -o ./aaa.S
riscv32-unknown-linux-gnu-gcc ./aaa.S sylib-riscv-linux.a -o aaa.out

if [ "$1" = "run" ]; then
  qemu-riscv32.sh ./aaa.out
fi