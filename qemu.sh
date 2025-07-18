#!/bin/sh

qemu-system-x86_64 \
    -bios /usr/share/OVMF/OVMF_CODE.fd \
    -machine q35 \
    -m 256M \
    -net none \
    -usb \
    -vga std \
    -name TEST-IMG \
    -drive file=test.img,format=raw \
    -k de \
    -display gtk \
    -device usb-mouse \
    -rtc base=localtime \
