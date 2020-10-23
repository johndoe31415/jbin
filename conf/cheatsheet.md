# Cheatsheet on how to fix some bugs I encounter more than once
This is a incoherent text document of bugs that I encounter, google, learn the
fix and then forget about again in four weeks and need to re-google. I'll try
to document them and other stuff so that I have it handy when I need it. Also
I'll describe some minimalist checklists that I hopefully use (e.g., when
installing a Linux system via debootstrap I *always* forget to change the root
passwd the first time around). It's probably not very useful for anyone else.


## debootstrap checklist
  * During fdisk, do not forget: BIOS boot (for GPT), /boot partition (for crypto-root)
  * debootstrap eoan subdir http://ftp.halifax.rwth-aachen.de/ubuntu/
  * debootstrap buster subdir http://ftp.de.debian.org/debian/
  * chmod 000 /etc/update-motd.d/10-help-text
  * apt-get update && apt-get install acl apt-file bzip2 convmv cryptsetup gnupg git grub2 hdparm hexedit inetutils-tools kbd less linux-image-generic lm-sensors locales lshw lsof lzma man mlocate nano openssh-client openssh-server p7zip-full pciutils psmisc python3 pwgen recode rsync screen sqlite3 unzip usbutils vim xz-utils net-tools
  * cd /usr; git clone https://github.com/johndoe31415/jbin
  * apt-get install ubuntu-mate-desktop
  * vim /etc/fstab
  * vim /etc/crypttab
  * vim /etc/hostname
  * adduser / passwd
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

Then: netplan apply


## Simple systemd user service
  * mkdir -p ~/.local/share/systemd/user
  * cat >~/.local/share/systemd/user/simple.service

```
[Unit]
Description=Simple service
After=network-online.target

[Service]
Type=simple
ExecStart=/home/joe/foo.py
WorkingDirectory=/home/joe/foo
# For system units not running as root
#User=nobody
#Group=nobody

[Install]
# For system units:
#WantedBy=multi-user.target
WantedBy=default.target
```

  * systemctl --user enable simple
  * loginctl enable-linger joe

## Postgres setup
  * CREATE DATABASE foodb;
  * CREATE USER foouser WITH ENCRYPTED PASSWORD 'foopass';
  * GRANT ALL PRIVILEGES ON DATABASE foodb TO foouser;
  * Show tables: \dt

Grant user read-only privileges
  * \c foodb
  * GRANT CONNECT ON DATABASE foodb TO foouser;
  * GRANT SELECT ON ALL TABLES IN SCHEMA public TO foouser;

Delete user
  * DROP OWNED BY foouser;
  * DROP USER foouser;

Change password
  * ALTER USER foouser WITH ENCRYPTED PASSWORD 'foopass';

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

## Complete rsync backup
rsync -aHAX --numeric-ids --exclude /proc --exclude /sys --exclude /tmp --exclude /dev --exclude /var/cache/apt root@server.com:/ backup

## Setting nameserver when using systemd-resolved
resolvectl dns eth0 8.8.8.8

## Useful tcpdump command lines
  * Monitor all DNS requests
    stdbuf -o 0 tcpdump -n -i eth0 "src 192.168.1.999 and udp and port 53" >dns_list.txt 2>&1

## /etc/sudoers
```
joe ALL=NOPASSWD: /usr/jbin/sbin/boot_windows
```

## Webcam
Device information:
v4l2-ctl --list-formats
v4l2-ctl --list-formats-ext

Playback:
mplayer -tv driver=v4l2:width=1280:height=720:outfmt=mjpg:device=/dev/video0 tv://

Recording audio and video:
ffmpeg -f alsa -i default -f video4linux2 -framerate 30 -video_size 1920x1080 -input_format mjpeg -i /dev/video0 audio_and_video.mkv

## Go
Get a new version (not the rancid old Ubuntu version), in particular with
module support ("go mod"). Download package and move whole go/ subdirectory to
/usr/local/bin/golang, then create symlinks:

```
# chown -R root:root /tmp/go
# mv /tmp/go /usr/local/bin/golang
# ln -s /usr/local/bin/golang/bin/go /usr/local/bin/go
# ln -s /usr/local/bin/golang/bin/gofmt /usr/local/bin/gofmt
```

To build, look for directories with "main.go" file, then "go build".

## LVM2
PV -> VG -> LV
Grow LV: lvresize -L +50G /dev/vg0/test-disk

## Python PyPi
python3 setup.py sdist
twine upload dist/foobar-1.2.3.tar.gz

## Mounting CIFS/Samba share
mount.cifs -o user=guest,pass=guest,uid=1000,gid=1000,dir_mode=0755,file_mode=0644 //sz/Users dos

## SMTP/MTA configuration check
  * Test email to check-auth@verifier.port25.com
  * [https://dkimvalidator.com/](https://dkimvalidator.com/)
  * Test email to Google

## Setting up Wireguard server
Create keys: wg genkey | tee /etc/wireguard/wg0-private.key | wg pubkey >/etc/wireguard/wg0-public.key

Server config `/etc/wireguard/wg0.conf`:
```
[Interface]
Address = 172.16.0.1/24
ListenPort = 51820
PrivateKey = uAnx1QjAP5ZZ5nj2irlcAuwrtlCA/dHl3zMvmrD4Fm4=

[Peer]
PublicKey = UxkQ/WOxDQtuta2C8UMhNMDn0GsYPdj8rhmwX0Yjfwo=
AllowedIPs = 172.16.0.2/32

[Peer]
PublicKey = /D3wtSoCwxU7KeChYhEH5TEBaDK3AnM6TRr4mkTNehk=
AllowedIPs = 172.16.0.3/32
```

Client config `/etc/wireguard/wg0.conf`:
```
[Interface]
PrivateKey = KNCROjUKuGCnQxIoCwEKmPYyXPVzAS6i9rtgK0Qttm4=
Address = 172.16.0.3/24

[Peer]
PublicKey = tT2zR6ChaZpWYx6L2B4T58laJKBOeE5Og9j8vJZJCW0=
AllowedIPs = 172.16.0.0/24
Endpoint = my-vpn-server.com:51820
PersistentKeepalive = 60
```

Enable systemd: systemctl enable wg-quick@wg0

## Ubuntu unused locales
Edit /var/lib/locales/supported.d/*
