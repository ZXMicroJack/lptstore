#!/bin/bash
dd if=/dev/zero of=card bs=1024 count=32768
mkfs.vfat -F 16 card
od -Ax -t x1 -w1 card | awk '/^[0-9]/ { printf("\t\tcase 0x%s: return 0x%s;\n", $1, $2); }' > fakedisk.h
