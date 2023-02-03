#!/bin/bash
dd if=/dev/zero of=card bs=1024 count=65536
mkfs.vfat -F 32 card
mkdir d
sudo mount -o loop card ./d
sudo chmod -R 777 d

dd if=/dev/urandom of=bios.dat bs=1024 count=64

### do copy
sudo dd if=/dev/zero of=d/CONFIG.BIN bs=1024 count=2
sudo cp ./bios.dat d/DISKA.DAT
sudo cp ./bios.dat d/DISKB.DAT
sudo cp ./bios.dat d/DISKC.DAT
sudo cp ./bios.dat d/DISKD.DAT
sudo cp ./bios.dat d/DISKE.DAT
sudo cp ./bios.dat d/DISKF.DAT
sudo cp ./bios.dat d/DISKG.DAT
sudo cp ./bios.dat d/DISKH.DAT
sudo cp ./bios.dat d/DISKI.DAT
sudo cp ./bios.dat d/DISKJ.DAT
sudo cp ./bios.dat d/DISKK.DAT
sudo cp ./bios.dat d/DISKL.DAT
sudo cp ./bios.dat d/DISKM.DAT
sudo cp ./bios.dat d/DISKN.DAT
sudo cp ./bios.dat d/DISKO.DAT
sudo cp ./bios.dat d/DISKP.DAT

sudo umount d
rm -rd d

chmod 777 card
# write to card
#sudo dd if=card of=/dev/mmcblk0

#sync
#sync
