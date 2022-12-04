#!/bin/sh
set -e
. ./build.sh

cd initrd
tar -cvf ../sysroot/boot/initrd.tar *
cd ..

dd if=/dev/zero of=disk.img bs=1M count=2
mkfs.vfat -F12 disk.img

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub
mkdir -p isodir/modules

cp modules/test_module.bin isodir/modules
cp -r sysroot/* isodir/
#cp sysroot/boot/kernel.kernel isodir/boot/kernel.kernel
cp grub/grub.cfg isodir/boot/grub/grub.cfg

if ! [[ -e "memtest86plus" ]]; then
echo "don't exist"
git clone https://github.com/memtest86plus/memtest86plus.git
fi

if [ ! -f memtest86plus/build32/memtest.bin ]; then
cd memtest86plus
cd build32
make
cd ../..
fi
cp memtest86plus/build32/memtest.bin isodir/boot/memtest.bin


grub-mkrescue /usr/lib/grub/i386-pc -o kernel.iso isodir
