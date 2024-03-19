FROM archlinux:latest

RUN pacman -Syu --noconfirm \
	&& pacman -S --noconfirm gcc clang flex bison ccache \
		pkg-config ninja automake autoconf libtool \
		wget curl git bc slib squashfs-tools \
		rsync jq cmake openssh tree parallel \
		python3 python-pip tcl scons dtc cpio \
		fakeroot fakechroot bzr ncurses cvs file \
		mercurial unzip subversion gawk diffstat \
		texinfo chrpath socat ckermit neovim zstd \
		cscope confuse dosfstools gettext e2fsprogs \
		make

RUN echo en_US.UTF-8 UTF-8 >> /etc/locale.gen
RUN locale-gen
