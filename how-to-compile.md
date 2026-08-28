[//]: # (Copyright (c) 2026 AmineTheJurk)
# JurkOS

This document explains how to compile the 1.0 Alpha release.

## How it Works
1.  **Kernel Entry Point**: We compile `main.c` statically. This ensures the binary has zero dependencies and can run as the very first process (PID 1) on the Linux kernel.
2.  **Image Creation**: A raw 512MB disk image is created using `truncate`.
3.  **Partitioning**: An MBR (Legacy BIOS) partition table is written to the image, and a primary `ext4` partition is created.
4.  **Deterministic PARTUUID**: We use `dd` to inject a specific Disk Signature (`12345678`) into the MBR. This allows the OS to find its own partition regardless of hardware configuration.
5.  **Filesystem & Injection**: The image is attached to a loop device, formatted as `ext4`, and all system files (BusyBox binaries, ASCII art, etc.) are injected into the drive.
6.  **Bootloader**: GRUB is installed into the MBR of the image and configured to boot the JurkOS kernel.

## 1. Install Required Tools
Before building, ensure your WSL/Linux environment has the following tools installed:

```bash
sudo apt update && sudo apt install -y gcc-multilib grub-pc-bin parted e2fsprogs
```

## 2. Build the Image
Navigate to the project root and run the following command:

```bash
gcc -static iso/sbin/main.c -o iso/sbin/main && truncate -s 512M jurkos.img && parted -s jurkos.img mklabel msdos mkpart primary ext4 1M 100% set 1 boot on && printf "\x78\x56\x34\x12" | dd of=jurkos.img bs=1 seek=440 conv=notrunc && DEVICE=$(sudo losetup -Pf --show jurkos.img) && sudo mkfs.ext4 -L JURKOS_ROOT ${DEVICE}p1 && sudo mkdir -p /mnt/jurk_tmp && sudo mount ${DEVICE}p1 /mnt/jurk_tmp && sudo cp -a iso/. /mnt/jurk_tmp/ && sudo grub-install --target=i386-pc --boot-directory=/mnt/jurk_tmp/boot $DEVICE && sudo umount /mnt/jurk_tmp && sudo losetup -d $DEVICE && echo "JurkOS 1.0 Alpha: Build Success."
```

## 3. Flash to USB
Once the build is complete, use **ImageUSB** to flash the `jurkos.img` to your physical USB drive. 

**Enjoy your fork!**
