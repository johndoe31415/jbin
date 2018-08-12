# Cheatsheet on how to fix some bugs I encounter more than once
This is a incoherent text document of bugs that I encounter, google, learn the
fix and then forget about again in four weeks and need to re-google. I'll try
to document them and other stuff so that I have it handy when I need it. Also
I'll describe some minimalist checklists that I hopefully use (e.g., when
installing a Linux system via debootstrap I *always* forget to change the root
passwd the first time around). It's probably not very useful for anyone else.


## debootstrap checklist
  * fdisk /boot partition
  * debootstrap
  * apt-get install acl apt-file bzip2 convmv gnupg grub2 hdparm hexedit inetutils-tools kbd less linux-image-generic lm-sensors locales lshw lsof lzma man mlocate nano openssh-client openssh-server p7zip-full pciutils psmisc pwgen recode rsync screen sqlite3 unzip usbutils vim xz-utils
  * fstab / crypttab / hostname
  * passwd
  * install-grub
  * update-grub
  * update-initramfs


## Bugfix: Ubuntu/Thunderbird shows huge emojis in subject line
  * apt-get install fonts-symbola
