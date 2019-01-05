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
