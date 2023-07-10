# bslock - simple screen locker
A better simple screen locker utility for X (Fork of slock)

## How is it different from slock
* A clean keypress feedback
* Xresources patch
* Caps lock patch
* Display text patch
* Show the last key action's time as a privacy feature


### Install it directly
You can clone this repository and install it by running `make install`. You may have to use sudo.
In order to build bslock you need the Xlib header files.
Edit config.mk to match your local setup if needed (bslock is installed into the /usr/local namespace by default).


## Running bslock
Simply invoke the 'bslock' command. To get out of it, enter your password.

