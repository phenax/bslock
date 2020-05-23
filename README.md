# bslock - simple screen locker
A better simple screen locker utility for X (Fork of slock)

![bslock demo](./media/demo.gif)

## How is it different from slock
* A clean keypress feedback
* Xresources patch
* Caps lock patch
* Display text patch
* Background image support (work in progress)


## Installation

### From AUR
Arch users can install [bslock](https://aur.archlinux.org/packages/bslock/) from AUR.
```bash
yay -S bslock
# OR
yaourt bslock
```

### Install it directly
You can clone this repository and install it by running `make install`. You may have to use sudo.
In order to build bslock you need the Xlib header files.
Edit config.mk to match your local setup if needed (bslock is installed into the /usr/local namespace by default).


## Running bslock
Simply invoke the 'bslock' command. To get out of it, enter your password.

