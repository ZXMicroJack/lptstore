#!/bin/bash
mkdir fd &> /dev/null
sudo mount -o loop dos33.img fd
if [ "$1" != "" ]; then
	bash
fi
sudo cp ./SDPP10/SD.SYS ./fd/LPTSTORE.SYS
sudo umount fd
qemu-system-i386 -fda dos33.img

