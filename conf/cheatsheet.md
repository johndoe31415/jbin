# Cheatsheet on how to fix some bugs I encounter more than once
This is a incoherent text document of bugs that I encounter, google, learn the
fix and then forget about again in four weeks and need to re-google. I'll try
to document them and other stuff so that I have it handy when I need it. Also
I'll describe some minimalist checklists that I hopefully use (e.g., when
installing a Linux system via debootstrap I *always* forget to change the root
passwd the first time around). It's probably not very useful for anyone else.


## debootstrap checklist
  * fdisk /boot partition
  * debootstrap cosmic subdir http://ftp.halifax.rwth-aachen.de/ubuntu/
  * debootstrap stretch subdir http://ftp.de.debian.org/debian/
  * chmod 000 /etc/update-motd.d/10-help-text 
  * apt-get install acl apt-file bzip2 convmv gnupg grub2 hdparm hexedit inetutils-tools kbd less linux-image-generic lm-sensors locales lshw lsof lzma man mlocate nano openssh-client openssh-server p7zip-full pciutils psmisc pwgen recode rsync screen sqlite3 unzip usbutils vim xz-utils net-tools
  * fstab / crypttab / hostname
  * passwd
  * grub-install
  * update-grub
  * update-initramfs


## Disable annoying Ubuntu crash reporter
  * apt-get --purge remove apport


## NetworkManager "eth0: unmanaged"
  * touch /etc/NetworkManager/conf.d/10-globally-managed-devices.conf
  * nmcli dev set eth0 managed yes


## systemd-resolved "Temporary failure in name resolution"
  * systemd-resolve --status
  * cat /run/systemd/resolve/resolv.conf
  * initramfs DHCP config with dhclient? apt-get install dhcpcd5


## netplan
/etc/netplan/01-custom.yaml
```
network:
    version: 2
    renderer: networkd
    ethernets:
        eth0:
            dhcp4: true
```

Then, `netplan apply`


## Simple systemd user service
  * mkdir -p ~/.local/share/systemd/user
  * cat >~/.local/share/systemd/user/simple.service
  * systemctl --user enable simple
  * loginctl enable-linger joe

```
[Unit]
Description=Simple service
After=network-online.target

[Service]
Type=simple
ExecStart=/home/joe/foo.py
WorkingDirectory=/home/joe/foo

[Install]
# For system units:
#WantedBy=multi-user.target
WantedBy=default.target
```


## Postgres setup
  * CREATE DATABASE foodb;
  * CREATE USER foouser WITH ENCRYPTED PASSWORD 'foopass';
  * GRANT ALL PRIVILEGES ON DATABASE foodb TO foouser;
  * Show tables: \dt

## Bugfix: Ubuntu/Thunderbird shows huge emojis in subject line
  * apt-get install fonts-symbola

## mkfs.ext4 without lazy init
  * mkfs.ext4 -E lazy_itable_init=0,lazy_journal_init=0

## kernel prerequisites for vanilla system
  * apt-get install build-essential flex bison libncurses5-dev libssl-dev bc libelf-dev

## GPT UEFI boot
  * apt-get install grub-efi-amd64 efivar
  * vfat partition type EFI System (~128 MiB), mount at /boot/efi
  * ext4 partition type Linux (~1 GiB), mount at /boot
  * modprobe efivars
  * mount -t efivarfs efivarfs /sys/firmware/efi/efivars
  * grub-install --efi-directory=/boot/efi --target=x86_64-efi /dev/sdx

## Using lirc/Iguanaworks USB to TX
  * TX works with stock kernel module
  * No need to install stupid Iguanaworks special software (igdaemon, etc.)
  * /etc/lirc/lirc_options.conf "driver = default" instead of "driver = devinput"
  * Place remote files in /etc/lirc/lircd.conf.d
  * irsend list "" ""
  * irsend list fooremote ""
  * irsend send_once fooremote KEY_UP

## Creating Windwows10 USB boot stick
  * Partition MBR:
     - one primary marked bootable, type 0xc
     - seocnd primary type 0x7
  * Format first partition with FAT32: mkfs.vfat -F32 /dev/sdx1
  * Format second partition with NTFS: mkfs.ntfs -Q /dev/sdx2
  * Loopmount ISO image to /tmp/iso and USB stick to /tmp/fat and /tmp/ntfs
  * rsync --exclude sources -r /tmp/iso/. /tmp/fat/
  * mkdir /tmp/fat/sources
  * cp /tmp/iso/sources/boot.wim /tmp/fat/sources
  * rsync -r /tmp/iso/. /tmp/ntfs/
  * umount /tmp/fat; umount /tmp/ntfs; sync
  * Boot using UEFI

## Setting nameserver when using systemd-resolved
`resolvectl dns eth0 8.8.8.8`

## Useful tcpdump command lines
  * Monitor all DNS requests
    `stdbuf -o 0 tcpdump -n -i eth0 "src 192.168.1.999 and udp and port 53" >dns_list.txt 2>&1`

## /etc/sudoers
```
joe ALL=NOPASSWD: /usr/jbin/sbin/boot_windows
```
