# RemoteSwitchController

## Prerequirement

Orange Pi Zero with Armbian

or

Rasberry Pi Zero


## Linux Headers

### Armbian

Append this line to /etc/apt/sources.list

```
deb http://apt.armbian.com bionic main
```

```sh
sudo apt update
sudo apt install linux-headers-next-sunxi64
# sudo ln -s /lib/modules/$(uname -r) /lib/modules/$(uname -r)/build
```

Workaround for armbian [armbian/build#1278](https://github.com/armbian/build/issues/1278)

```sh
cd /usr/src/linux-headers-$(uname -r)
sudo mkdir net/wireguard && sudo touch net/wireguard/{Kconfig,Makefile}
sudo make scripts
```

## Compile

```sh
make
```