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
  * apt-get update && apt-get install acl apt-file bzip2 convmv cryptsetup gnupg git grub2 hdparm hexedit inetutils-tools kbd less linux-image-generic lm-sensors locales lshw lsof lzma man locate nano openssh-client openssh-server p7zip-full pciutils psmisc python3 pwgen recode rsync screen sqlite3 unzip usbutils vim xz-utils net-tools debsums
  * cd /usr; git clone https://github.com/johndoe31415/jbin
  * apt-get install ubuntu-mate-desktop
  * vim /etc/fstab
  * vim /etc/crypttab
  * vim /etc/hostname
  * adduser / passwd
  * grub-install
  * update-grub
  * update-initramfs


## Disable annoying Ubuntu crash reporter and other garbage
  * apt-get --purge remove apport command-not-found


## NetworkManager "eth0: unmanaged"
  * touch /etc/NetworkManager/conf.d/10-globally-managed-devices.conf
  * nmcli dev set eth0 managed yes

To disable NetworkManager from managing an interface:

	* nmcli dev set eth0 managed no


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
  * Show databases: \l
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

Drop all tables owned by user
  * DROP OWNED BY foouser;

## MariaDB setup
  * CREATE DATABASE foodb;
  * CREATE USER foouser IDENTIFIED BY 'foopass';
  * GRANT ALL privileges ON foodb.* TO foouser;

Show all users
  * SELECT user, host FROM mysql.user;

Change user password
  * ALTER USER foouser IDENTIFIED BY 'foopass';
  * FLUSH PRIVILEGES;

Make a dump without "definer" so we can re-import without SUPER privileges
  * mysqldump mytable | grep -vE '^/\*!50013 DEFINER=.* SQL SECURITY DEFINER \*/$' >mytable.sql

Only dump the data
  * mysqldump --single-transaction --no-create-db --no-create-info mytable | grep -vE '^/\*!50013 DEFINER=.* SQL SECURITY DEFINER \*/$' >mytabledata.sql

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

## webcam
Device information:
v4l2-ctl --list-formats
v4l2-ctl --list-formats-ext

Playback mplayer:
mplayer -tv driver=v4l2:width=1280:height=720:outfmt=mjpg:device=/dev/video0 tv://

Playback vlc:
vlc v4l2:// :live-caching=300 :v4l2-width=1280 :v4l2-height=720 :v4l2-fps=30 :v4l2-chroma=mjpg

Recording audio and video:
ffmpeg -f alsa -i default -f video4linux2 -framerate 30 -video_size 1920x1080 -input_format mjpeg -i /dev/video0 audio_and_video.mkv

Setting manual focus:
v4l2-ctl -d /dev/video2 --set-ctrl=focus_absolute=14

Streaming webcam to HTTP:
cvlc v4l2:// :v4l2-standard= :v4l2-dev=/dev/video0 :v4l2-chroma=MJPG :v4l2-width=1920 :v4l2-height=1080 :v4l2-fps=30 :v4l2-controls-reset :live-caching=300 --sout '#standard{access=http,mux=ogg,dst=127.0.0.1:8111}'

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
Scan for LVM devices: lvmdiskscan
Scan for PVs, VGs, LVs: pvscan / vgscan --mknodes / lvscan
Initially creating the PV: pvcreate /dev/sdx1
Show PVs: pvdisplay
Then creating the VG on PV or PVs: vgcreate vg0 /dev/sdx1 /dev/sdy1
Creating LV: lvcreate -L 20G -n mydisk vg0
Creating LV with striping: lvcreate -L 100%FREE -n stripeddisk -i2 vg0
Grow LV: lvresize -L +50G /dev/vg0/mydisk
Create snapshot with absolute size: lvcreate -s -L 2G -n mysnap /dev/vg0/mydisk
Create snapshot with relative size: lvcreate -s -l 50%FREE -n mysnap /dev/vg0/mydisk
Remove LV (or snapshot): lvremove /dev/vg0/mysnap
Revert snapshot to original: lvconvert --merge /dev/vg0/mysnap
Deactivate disk: lvchange -a n /dev/vg0/mydisk
Activate disk: lvchange -a y /dev/vg0/mydisk

## Python PyPi
python3 setup.py install --user		# For testing
python3 setup.py sdist
twine upload dist/foobar-1.2.3.tar.gz

## Python venv
python3 -m venv "${HOME}/.local"

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

## Monitor sources
EDID protocol control via ddcutil
Show supported modes: get-edid | parse-edid
Show monitors: ddcutil detect
Show all VCP properties: ddcutil vcpinfo

Example property: 0x60 ("Input Source")
Show particular VCP property: ddcutil vcpinfo 60 -v
Set a particular input source (set display 2 to DisplayPort-1): ddcutil setvcp -d 2 60 0x0f

Alternative example property: 0x10 ("Brightness")
Set brightness to 15 on left and right monitors:
ddcutil setvcp 10 15 -d 1 && ddcutil setvcp 10 15 -d 2

Set brightness to 80 on left and right monitors:
ddcutil setvcp 10 80 -d 1 && ddcutil setvcp 10 80 -d 2

Disable monitor hotplug events in Mate: gsettings set org.mate.SettingsDaemon.plugins.xrandr active false

## HBCI PIN/TAN configuration Comdirect
USER_ID="12345670"
BIC="COBADEHD001"
BLZ="20041133"
NAME="comdirect"
UNIQUE_ID="1"
UNIQUE_ACCOUNT_ID="2"

aqhbci-tool4 adduser -t pintan -u "$USER_ID" -s https://fints.comdirect.de/fints -N "$NAME" -b "$BLZ"
aqhbci-tool4 listaccounts
aqhbci-tool4 getsysid -u "$UNIQUE_ID"
aqhbci-tool4 listitanmodes -u "$UNIQUE_ID"
aqhbci-tool4 setitanmode -u "$UNIQUE_ID" -m 6902
aqhbci-tool4 getaccounts -u "$UNIQUE_ID"

aqhbci-tool4 listaccounts -v
aqbanking-cli request --aid="$UNIQUE_ACCOUNT_ID" -c "output_data.txt" --transactions

## C str(n) variant cheatsheet
char buffer[128];
sizeof(buffer) - 1: strncpy
sizeof(buffer): snprintf, fgets

## Headless VNC Server
apt-get install tigervncserver mate-desktop-environment
vncpasswd
vncserver -localhost no -geometry 1900x900

## Debian /etc/network/interfaces
auto eth0
allow-hotplug eth0
iface eth0 inet dhcp

auto eth0
allow-hotplug eth0
iface eth0 inet static
	address 192.168.1.99/24
	gateway 192.168.1.1

auto eth0.123
allow-hotplug eth0.123
iface eth0.123 inet static
	address 192.168.2.99/24
	vlan-raw-device eth0
	mtu 1496

## Prevent renaming of VLAN devices
cat >/etc/systemd/network/10-vlan.link
[Match]
Driver=*802.1Q*
[Link]
NamePolicy=kernel

update-initramfs -u

## Rename network interface temporarily
ifconfig enx12345678 down
ip link set enx12345678 name eth9

## VLAN with ip instead of vconfig

ip link add link eth0 name eth0.100 type vlan id 100
ip -d link show eth0.100
ip addr add 192.168.100.1/24 brd 192.168.100.255 dev eth0.100
ip link set dev eth0.100 up

ip link delete eth0.100

## Enable HDMI at bootup, even when unplugged
Kernel commandline: video=HDMI-1:e

## Show grub menu
/etc/default/grub
GRUB_TIMEOUT_STYLE=menu

## KVM virutalisation

Installation of packets:
apt-get install libvirt-daemon libvirt-daemon-system libvirt-clients virtinst libosinfo-bin

Installation of UI:
apt-get install virt-manager virt-viewer

Listing all current Domains:
virsh list

Installing a new system:
virt-install --connect qemu:///system --name testvm --ram 1024 --os-type=linux --os-variant=debian10 --location http://ftp.de.debian.org/debian/dists/stable/main/installer-amd64/ --accelerate --disk path=/tmp/foo --vnc --noautoconsole --network network=default

Connecting to a system:
virt-viewer --connect qemu:///system testvm
virt-viewer --connect qemu+ssh://joe@127.0.0.1/system testvm
virsh --connect qemu:///system console testvm

Dumping the configuration:
virsh dumpxml testvm

Attaching a CDROM:
virsh attach-disk testvm /home/joe/grml64.iso hdc  --type cdrom --mode readonly --targetbus sata --config
virsh change-media testvm hdc --eject
virsh change-media testvm hdc --source /home/joe/grm64.iso

Starting and stopping VM:
virsh start testvm
virsh destroy testvm

Create new network:
<network>
	<name>internal</name>
	<uuid>f9a1544d-3adb-4a96-9e5c-df2ce4d3407c</uuid>
</network>

virsh net-create internal.xml

Using the UI: virt-manager

## Debugging Routing Decision
ip route show to match 1.2.3.4
ip route get to 1.2.3.4 from 2.3.4.5 iif eth0

## Resetting Mate and removing all custom configuration
Remove:

~/.config/autostart
~/.config/caja
~/.config/dconf
~/.config/mate
~/.config/mate-session
~/.config/menus

## Editing the Mate menu
mozo

## Where the Mate menu is stored
*.desktop starters: ~/.local/share/applications
*.directory directories: ~/.local/share/desktop-directories
Tying them together: ~/.config/menus/mate-applications.menu

## Using a different ssh key for git
export GIT_SSH_COMMAND="ssh -i ~/testkey"

## Remapping keys using systemd-hwdb
Note: Hex IDs are case sensitive (need to be uppercase). Query the systemd
hardware database for a particular device:

systemd-hwdb query evdev:input:b0003v046DpC52D

Where v = USB vendor ID, p = USB product ID

Finding scancodes of a particular device under an actual console:

showkey --scancodes

Finding scancodes under X:

evtest

List X input devices: xinput list
See if we chose the right device: xinput test 12
List properties: xinput list-props 12

## systemd-hwdb
Priority of files: sorting order of combined directories:

  * /lib/udev/hwdb.d
  * /etc/udev/hwdb.d

## Find modalias of device
First, check /sys/bus/usb/devices/; directories are not named like
bus/devicenum, so a bit of poking around is necessary. For example

reliant [~]: cat /sys/bus/usb/devices/3-7/busnum 
3
reliant [~]: cat /sys/bus/usb/devices/3-7/devnum 
14
reliant [~]: cat /sys/bus/usb/devices/3-7/manufacturer 
Logitech
reliant [~]: cat /sys/bus/usb/devices/3-7/product 
USB Receiver

Shows that USB device 003:014 is found at /sys/bus/usb/devices/3-7. Then, see
the endpoints, which are subdirectories within:

ls /sys/bus/usb/devices/3-7/

drwxr-xr-x 6 root root    0   08.07.2021 22:09:32 3-7:1.0/
drwxr-xr-x 6 root root    0   08.07.2021 22:09:32 3-7:1.1/
[...]

Choose the right one (e.g., in this case one is mouse emulation, the other is
keyboard emulation).

reliant [~]: cat /sys/bus/usb/devices/3-7/3-7:1.0/modalias
usb:v046DpC52Dd1701dc00dsc00dp00ic03isc01ip01in00

## Find modalias of evdev
Verify we know the correct event device:

udevadm info /dev/input/event3

Then:

cat /sys/class/input/event3/device/modalias 
input:b0003v046DpC52De0111-e0,1,2,3,4,14,k71,72,73,74,[...]

Everything before the "e" is the modalias string (the rest is version
information and supported keys). In this case: input:b0003v046DpC52D

## Getting Logitech R400 presenting buttons work
cat >/etc/udev/hwdb.d/70-logitech-r400.hwdb <<EOF
evdev:input:b0003v046DpC52D*
 KEYBOARD_KEY_070029=f
 KEYBOARD_KEY_07003e=f
 KEYBOARD_KEY_070037=g
EOF

systemd-hwdb update
udevadm trigger

## Ubuntu RFkill for regular users
Add to group: netdev

## Enable/Disable AppArmor
As root:
aa-disable /usr/bin/evince
aa-enforce /usr/bin/evince

## Docker
  * docker images						# List images
  * docker build -t myimage myimage		# Build from Dockerfile in myimage/ and tag as "myimage"
  * docker rmi myimage					# Remove image
  * docker run -ti myimage bash			# Run image with terminal
  * docker ps							# Get running containers/container IDs
  * docker ps -a						# Get all containers/container IDs (even dead ones)
  * docker container prune				# Remove dead containers
  * docker commit 49ba476abbb0 foo		# Commit changes to the given tag
  * docker history foo					# Show what led to a container
  * docker exec -ti 0cdea257064f bash	# Get a terminal within a container
  * docker start 0cdea257064f			# Start a dead container
  * docker attach 0cdea257064f			# Attach a terminal to that container
  * docker network ls					# Show the network
  * docker network inspect bridge		# Show network details
  * Create non-NAT docker network interface:
    docker network create -d bridge --opt com.docker.network.bridge.name=docker1 --opt com.docker.network.bridge.enable_icc=false --opt com.docker.network.bridge.enable_ip_masquerade=false --opt com.docker.network.bridge.host_binding_ipv4=172.18.0.1 --subnet=172.18.0.0/16 nonat
  * Restrict docker container to only a single host port
    iptables -A INPUT '!' -p tcp -i docker1 -j REJECT
    iptables -A INPUT -p tcp -i docker1 '!' --destination-port 1234 -j REJECT
  * Start container without outside connectivity:
    docker run -it --network nonat --dns 0.0.0.0 --dns-search localdomain myimage
  * Start a container using an overlay
    docker create my_image ls /
	(returns container ID)
    docker start 81326a713e4df606adc2f2788811fc79530c870a309842e6eb3b041b1e4e2707
	docker inspect 81326a713e4df606adc2f2788811fc79530c870a309842e6eb3b041b1e4e2707
	docker wait 81326a713e4df606adc2f2788811fc79530c870a309842e6eb3b041b1e4e2707
	docker logs 81326a713e4df606adc2f2788811fc79530c870a309842e6eb3b041b1e4e2707

## Window Compositors
Default Mate compositor: marco
With glx: compton
Switch at Menu -> Config -> MATE Tweak -> Windows

## Git
Removing deceased remote branches: git remote prune origin

## Download Signal manually
If you want Signal .deb files without needing to add them as an apt repository:

curl -s https://updates.signal.org/desktop/apt/dists/xenial/main/binary-amd64/Packages | grep ^Filename | grep -v beta | awk '{print "https://updates.signal.org/desktop/apt/" $2}'

## PulseAudio
Create pseudo-source that duplicates the left channel:

pacmd load-module module-remap-source source_name=EdirolMono master="alsa_input.usb-Roland_EDIROL_UA-25-00.analog-stereo" master_channel_map=front-left,front-left channel_map=front-left,front-right source_properties='device.description="EDIROL\ UA-25\ Duplicated\ Left\ Channel\ Mono\ Source"'

## Static ARP reply
ARP reply for an address that's not the IP of any NIC
arp -Ds 123.123.123.123 eth0 pub

## Libreoffice clear permanent text formatting
Format -> Clear Direct Formatting
Then search/replace regex .* by &

## Disable SNAP updates on bootup for 89 days
cat >/etc/systemd/system/snap-inhibit-updates.service <<EOF
[Unit]
Description=Inhibit SNAP updates for the next 90 days
After=snapd.service

[Service]
Type=oneshot
ExecStart=/bin/sh -c 'snap set system refresh.hold=`/bin/date -Isec -d "today+89 day"`'

[Install]
WantedBy=multi-user.target
EOF

systemctl enable snap-inhibit-updates.service && systemctl start snap-inhibit-updates.service

## Mirror website with fixing links
wget -mEkp --no-parent https://google.de

## LuaLaTeX
List all fonts: luaotfload-tool --list='*'
Regenerate font list: luaotfload-tool --update

## repo
repo forall -c "git clean -dfx"
repo forall -c "git reset hard"

## OP-TEE
repo init -u https://github.com/OP-TEE/manifest.git
repo sync
repo forall -c "git checkout 3.16.0"
cd build
make toolchains
make -j32 QEMU_VIRTFS_ENABLE=y QEMU_USERNET_ENABLE=y run


repo init -u https://github.com/OP-TEE/manifest.git -b 3.16.0

## Kernel keyring
Add specific key: keyctl add user mykey 7bdbf8b4feb337afae5d7848322a80a0 @u
Show keyring: keyctl show @u
Add trusted key: keyctl add trusted kek "new 32" @u

## PKCS#11
Initialize token: pkcs11-tool --init-token --label testtoken --so-pin 1234567890
Initialize user PIN: pkcs11-tool --label testtoken --login --so-pin 1234567890 --init-pin --pin 12345
List slots: pkcs11-tool -L
List mechanisms: pkcs11-tool -M
Generate symmetric key: pkcs11-tool -l --pin 12345 --keygen --key-type AES:16 --label symm_key
Generate asymmetric keypair: pkcs11-tool -l --pin 12345 --keypairgen --key-type EC:prime256v1 --label ec_key
Generate asymmetric keypair: pkcs11-tool -l --pin 12345 --keypairgen --key-type rsa:2048 --label rsa_key

## DBus
busctl list --user
busctl tree --user
busctl monitor --user
GUI: d-feet
dbus-send --print-reply --dest=org.cinnamon.Muffin.IdleMonitor /org/cinnamon/Muffin/IdleMonitor/Core org.cinnamon.Muffin.IdleMonitor.GetIdletime

# Get WPA2-Enterprise X.509 certificates
wpa_cli

## Bochs
./configure --enable-x86-64 --enable-smp --enable-debugger --enable-x86-debugger --enable-usb --with-wx
Show GDT: info gdt
Show paging info: info tab
CPU mode switch breakpoint: modebp
Dump linear address: x/32b 0x7c00
Dump physical address: xp/32b 0x7c00

## Python editable package
pip install -e .

## Evince zoom level
Need to increase page cache size, e.g. to 4 GiB
gsettings set org.gnome.Evince page-cache-size 4096

## KDE get all shortcut UUIDs
qdbus org.kde.kglobalaccel /component/khotkeys shortcutNames

## KDE find correct shortcut UUID
less ~/.config/khotkeysrc

## KDE execute shortcut
qdbus org.kde.kglobalaccel /component/khotkeys invokeShortcut {13c7148c-2439-41fc-9d48-6a5839ee4205}

## KDE execute shortcut on Meta (Super_L)
kwriteconfig5 --file ~/.config/kwinrc --group ModifierOnlyShortcuts --key Meta "org.kde.kglobalaccel,/component/khotkeys,org.kde.kglobalaccel.Component,invokeShortcut,{13c7148c-2439-41fc-9d48-6a5839ee4205}"
qdbus org.kde.KWin /KWin reconfigure

## TP-Link Serial Console
picocom --baud 115200 --omap delbs --imap bsdel /dev/ttyACM0
show interface vlan 999
show vlan
show interface status

## Vim XML file editing
In visual mode:
  - at (select block)
  - it (select inner block)
  - atato (jump to parent)

## storcli
  - Overview: ./storcli64 /c0 show all
  - Overview as JSON: ./storcli64 /c0 show all j
  - Detailed disk info: ./storcli64 /c0 /e8 /s7 show all
  - Move c0:e8:s7 from UBad to UGood: ./storcli64 /c0 /e8 /s7 set good
  - Add foreign disks to pool: ./storcli64 /c0 /fall import
  - Rebuild progress: ./storcli64 /c0 /eall /sall show rebuild
  - Smartctl with MegaRAID: smartctl --scan
  - smartctl -a -d megaraid,13 /dev/sda

## Bind9
Create TSIG key for record update: tsig-keygen
Use that file as keyfile for nsupdate as well as inside named.conf
Trigger an update:
```
update delete joe.dyn.spornkuller.de A
update add joe.dyn.spornkuller.de 10 A 2.2.3.4
send
```

`nsupdate -k keyfile.txt <nsupdate_cmds.txt`

## Download audio playlist
yt-dlp -x --audio-format mp3 -o '%(playlist_index)02d - %(title)s.mp3' 'https://www.youtube.com/watch?v=Q6bmUrTLaJk&list=OLAK5uy_n5iq5UJJUH_-9jgXNBo8S2w6f6xpYB7sA'

## Make annoying Snap STFU
snap refresh --hold=forever     # Disable auto-update
snap refresh                    # Trigger update
snap refresh --unhold           # Make Snap annoying again

## Record screen with ffmpeg
#ffmpeg -video_size 1024x768 -framerate 25 -f x11grab -i :0.0+100,200 -f pulse -ac 2 -i default output.mkv
ffmpeg -video_size 1024x768 -framerate 25 -f x11grab -i :0.0+0,0 -vf setpts=N/FR/TB output.mkv
