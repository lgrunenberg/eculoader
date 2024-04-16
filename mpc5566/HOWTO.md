# Compiler installation

Compiler situation on Ubuntu is "I give up" so here's a workaround with chroot.
( You can use this on Debian too if you don't want to install old crusty compilers )

> Problem with mpc5566 is that it's ***SPE*** and not regular powerpc so you can ***NOT*** use the vanilla compiler.

Install debootstrap
```
apt-get install debootstrap
```

Go to wherever you want the chroot to be

```
mkdir spe-chroot
debootstrap buster spe-chroot http://archive.debian.org/debian/
```


Create mount folder for project
```
mkdir -p spe-chroot/projects/eculoader
```

Mount project folder in chroot
```
mount --bind /path/to/project spe-chroot/projects/eculoader
```

Enter chroot
```
chroot spe-chroot
```

Install spe toolchains
```
apt-get install gcc-powerpc-linux-gnuspe make
```

Do stuff
```
cd /projects/eculoader/mpc5566
make
```

Don't forget to unmount after you've exited the chroot
```
umount spe-chroot/projects/eculoader
```
