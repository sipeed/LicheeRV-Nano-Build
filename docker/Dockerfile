FROM ubuntu:22.04

ENV DEBIAN_FRONTEND noninteractive

COPY libssl.so.1.1 /usr/local/lib/
COPY libcrypto.so.1.1 /usr/local/lib/

RUN ldconfig && apt update -y && \
	apt install -y pkg-config build-essential ninja-build\
	automake autoconf libtool wget curl git gcc libssl-dev bc\
	slib squashfs-tools android-sdk-libsparse-utils rsync\
	jq cmake python3-distutils tclsh scons parallel ssh-client tree\
	python3-dev python3-pip device-tree-compiler libssl-dev ssh cpio\
	squashfs-tools fakeroot libncurses5-dev flex bison\
	android-libext4-utils bdebstrap proot fakeroot fakechroot\
	bzr ca-certificates cvs file locales mercurial python3\
	python3-flake8 python3-nose2 python3-pexpect subversion\
	unzip ccache gawk diffstat texinfo chrpath socat\
	python3-pip python3-pexpect xz-utils debianutils\
	iputils-ping python3-git python3-jinja2 python3-subunit\
	zstd liblz4-tool libacl1 parted erofs-utils genext2fs mtools\
	fatcat ckermit neovim cscope asciinema libconfuse-dev dosfstools\
	gettext autoconf-archive

RUN echo en_US.UTF-8 UTF-8 >> /etc/locale.gen
RUN locale-gen
