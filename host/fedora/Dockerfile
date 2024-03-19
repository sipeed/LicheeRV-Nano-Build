FROM fedora:latest

RUN echo en_US.UTF-8 UTF-8 >> /etc/locale.gen

RUN dnf update -y && \
	dnf install -y pkg-config ninja-build \
	automake autoconf libtool wget curl git gcc gcc-c++ \
	slib squashfs-tools rsync jq cmake tcl scons parallel \
	openssh tree python-devel dtc openssl-devel cpio \
	squashfs-tools fakeroot ncurses-devel flex bison \
	fakechroot bzr cvs file mercurial subversion \
	unzip ccache gawk diffstat texinfo chrpath socat\
	xz zstd lz4 libacl parted mtools ckermit neovim \
	cscope asciinema libconfuse-devel dosfstools \
	gettext autoconf-archive clang bc
