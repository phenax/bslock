# bslock - simple screen locker
A better simple screen locker utility for X (Fork of slock)

## How is it different from slock
* A clean keypress feedback
* Background image support (work in progress)
* Xresources patch
* Caps lock patch
* Display text patch


## Requirements
In order to build slock you need the Xlib header files.


## Installation
Edit config.mk to match your local setup (slock is installed into
the /usr/local namespace by default).

Afterwards enter the following command to build and install slock
(if necessary as root):
```bash
make install
```


## Running slock
Simply invoke the 'slock' command. To get out of it, enter your password.


