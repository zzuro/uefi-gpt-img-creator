#!/bin/sh

qemu-system-x86_64 \
    -bios /usr/share/OVMF/OVMF_CODE.fd \
    -machine q35 \
    -net none \
    -drive file=test.img,format=raw \
    -k de \
    -display gtk \
