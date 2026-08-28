# JurkOS

```text
+---------------------------------------+
|                                       |
|            _______________            |
|           |_______________|           |
|                 ||||||                |
|                 ||||||                |
|                 ||||||   urkos        |
|                 ||||||                |
|                 ||||||                |
|          ////   ||||||                |
|         |||||  |||||||                |
|          ||||||||||||/                |
|           \_________/                 |
|                                       |
+---------------------------------------+
```

JurkOS is an independent, minimalist **"On-the-Go"** operating system targeting high-performance portability and bare-metal reliability. Designed to run directly from a physical flash drive, it prioritizes speed, efficiency, and a direct user-to-kernel relationship.

## Key Features

*   **Bare-Metal Independence**: Runs directly on physical hardware without the bloat of traditional distributions.
*   **Persistent Environment**: Full read-write support ensures your data and settings survive reboots.
*   **Dynamic Storage Expansion**: Automatically surgical-resizes to fill your entire USB stick (e.g., 16GB) on first boot.
*   **Deterministic Booting**: Uses a fixed PARTUUID method to ensure hardware independence across HDD, SSD, and NVMe.
*   **Kernel Shell**: A specialized C-based shell wrapping a powerful BusyBox ecosystem.
*   **Encrypted Identity**: Local persistence for user credentials and home directories.

## Roadmap

The Alpha release serves as the foundation for a fully independent ecosystem.
*   **Current**: 1.0 Alpha (32-bit & 64-bit x86 Support).
*   **Future (1.1 - 1.9 Alpha)**: Expanded hardware compatibility, driver optimizations, and more built-in system tools.
*   **Long-term**: Introduction of **ARM support** and tools to revive older computers.

## How It Works

1.  **MBR Partitioning**: The build process injects a custom **Disk Signature** (`12345678`) at offset 440 of the MBR.
2.  **Deterministic Mounting**: GRUB is configured to mount `PARTUUID=12345678-01`, ensuring the OS always finds its home.
3.  **Init (PID 1)**: The kernel hands control to a statically compiled `main.c` binary which initializes the system environment.
4.  **Surgical Setup**: JurkOS remounts the root as Read-Write, mounts virtual filesystems (`/proc`, `/sys`, `/dev`), and populates device nodes.
5.  **Filesystem Growth**: Calls `resize2fs` on the first boot to grow the ext4 filesystem to the physical limit of the drive.

## How To Compile

### 1. Install Required Tools
Ensure your WSL/Linux environment is ready:
```bash
sudo apt update && sudo apt install -y gcc-multilib grub-pc-bin parted e2fsprogs
```

### 2. Build the Image
Run the following command from the project root:
```bash
gcc -static iso/sbin/main.c -o iso/sbin/main && truncate -s 512M jurkos.img && parted -s jurkos.img mklabel msdos mkpart primary ext4 1M 100% set 1 boot on && printf "\x78\x56\x34\x12" | dd of=jurkos.img bs=1 seek=440 conv=notrunc && DEVICE=$(sudo losetup -Pf --show jurkos.img) && sudo mkfs.ext4 -L JURKOS_ROOT ${DEVICE}p1 && sudo mkdir -p /mnt/jurk_tmp && sudo mount ${DEVICE}p1 /mnt/jurk_tmp && sudo cp -a iso/. /mnt/jurk_tmp/ && sudo grub-install --target=i386-pc --boot-directory=/mnt/jurk_tmp/boot $DEVICE && sudo umount /mnt/jurk_tmp && sudo losetup -d $DEVICE && echo "JurkOS 1.0 Alpha: Build Success."
```

## Usage

1.  Flash the resulting `jurkos.img` to your physical USB drive using **ImageUSB**.
2.  Boot from the USB on a Legacy BIOS compatible machine (tested on ThinkPad X390).
3.  Enjoy a raw, powerful interface that puts you back in control.

## Hardware Disclaimer
Not all hardware is compatible yet. JurkOS is an early release and a fun project aimed at exploring the depths of OS development. If your hardware doesn't work, stay tuned for future Alpha updates! ;)

---
*JurkOS: High-performance, bare-metal, independent.*
