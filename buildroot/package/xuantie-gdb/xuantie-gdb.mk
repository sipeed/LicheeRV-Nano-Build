################################################################################
#
# xuantie-gdb
#
################################################################################

XUANTIE_GDB_VERSION = 3f1c1bda96771d78453da163ef7aedca540d0f18
XUANTIE_GDB_SITE = $(call github,T-head-Semi,binutils-gdb,$(XUANTIE_GDB_VERSION))

XUANTIE_GDB_LICENSE = GPL-2.0+, LGPL-2.0+, GPL-3.0+, LGPL-3.0+
XUANTIE_GDB_LICENSE_FILES = COPYING COPYING.LIB COPYING3 COPYING3.LIB
XUANTIE_GDB_CPE_ID_VENDOR = gnu

# Out of tree build is mandatory, so we create a 'build' subdirectory
# in the gdb sources, and build from there.
XUANTIE_GDB_SUBDIR = build
define XUANTIE_GDB_CONFIGURE_SYMLINK
	mkdir -p $(@D)/$(XUANTIE_GDB_SUBDIR)
	ln -sf ../configure $(@D)/$(XUANTIE_GDB_SUBDIR)/configure
endef
XUANTIE_GDB_PRE_CONFIGURE_HOOKS += XUANTIE_GDB_CONFIGURE_SYMLINK

# For the host variant, we really want to build with XML support,
# which is needed to read XML descriptions of target architectures. We
# also need ncurses.
# As for libiberty, gdb may use a system-installed one if present, so
# we must ensure ours is installed first.
HOST_XUANTIE_GDB_DEPENDENCIES = host-expat host-libiberty host-ncurses host-zlib

# Disable building documentation
XUANTIE_GDB_MAKE_OPTS += MAKEINFO=true
XUANTIE_GDB_INSTALL_TARGET_OPTS += MAKEINFO=true DESTDIR=$(TARGET_DIR) install
HOST_XUANTIE_GDB_MAKE_OPTS += MAKEINFO=true
HOST_XUANTIE_GDB_INSTALL_OPTS += MAKEINFO=true install

XUANTIE_GDB_DEPENDENCIES += host-flex host-bison
HOST_XUANTIE_GDB_DEPENDENCIES += host-flex host-bison

# When gdb sources are fetched from the binutils-gdb repository, they
# also contain the binutils sources, but binutils shouldn't be built,
# so we disable it (additionally the option --disable-install-libbfd
# prevents the un-wanted installation of libobcodes.so and libbfd.so).
XUANTIE_GDB_DISABLE_BINUTILS_CONF_OPTS = \
	--disable-binutils \
	--disable-install-libbfd \
	--disable-ld \
	--disable-gas \
	--disable-gprof

XUANTIE_GDB_CONF_ENV = \
	ac_cv_type_uintptr_t=yes \
	gt_cv_func_gettext_libintl=yes \
	ac_cv_func_dcgettext=yes \
	gdb_cv_func_sigsetjmp=yes \
	bash_cv_func_strcoll_broken=no \
	bash_cv_must_reinstall_sighandlers=no \
	bash_cv_func_sigsetjmp=present \
	bash_cv_have_mbstate_t=yes \
	gdb_cv_func_sigsetjmp=yes

# Starting with gdb 7.11, the bundled gnulib tries to use
# rpl_gettimeofday (gettimeofday replacement) due to the code being
# unable to determine if the replacement function should be used or
# not when cross-compiling with uClibc or musl as C libraries. So use
# gl_cv_func_gettimeofday_clobber=no to not use rpl_gettimeofday,
# assuming musl and uClibc have a properly working gettimeofday
# implementation. It needs to be passed to XUANTIE_GDB_CONF_ENV to build
# gdbserver only but also to XUANTIE_GDB_MAKE_ENV, because otherwise it does
# not get passed to the configure script of nested packages while
# building gdbserver with full debugger.
XUANTIE_GDB_CONF_ENV += gl_cv_func_gettimeofday_clobber=no
XUANTIE_GDB_MAKE_ENV += gl_cv_func_gettimeofday_clobber=no

# Similarly, starting with gdb 8.1, the bundled gnulib tries to use
# rpl_strerror. Let's tell gnulib the C library implementation works
# well enough.
XUANTIE_GDB_CONF_ENV += \
	gl_cv_func_working_strerror=yes \
	gl_cv_func_strerror_0_works=yes
XUANTIE_GDB_MAKE_ENV += \
	gl_cv_func_working_strerror=yes \
	gl_cv_func_strerror_0_works=yes

# Starting with glibc 2.25, the proc_service.h header has been copied
# from gdb to glibc so other tools can use it. However, that makes it
# necessary to make sure that declaration of prfpregset_t declaration
# is consistent between gdb and glibc. In gdb, however, there is a
# workaround for a broken prfpregset_t declaration in glibc 2.3 which
# uses AC_TRY_RUN to detect if it's needed, which doesn't work in
# cross-compilation. So pass the cache option to configure.
# It needs to be passed to XUANTIE_GDB_CONF_ENV to build gdbserver only but
# also to XUANTIE_GDB_MAKE_ENV, because otherwise it does not get passed to the
# configure script of nested packages while building gdbserver with full
# debugger.
XUANTIE_GDB_CONF_ENV += gdb_cv_prfpregset_t_broken=no
XUANTIE_GDB_MAKE_ENV += gdb_cv_prfpregset_t_broken=no

# We want the built-in libraries of gdb (libbfd, libopcodes) to be
# built and linked statically, as we do not install them on the
# target, to not clash with the ones potentially installed by
# binutils. This is why we pass --enable-static --disable-shared.
XUANTIE_GDB_CONF_OPTS = \
	--without-uiout \
	--disable-gdbtk \
	--without-x \
	--disable-sim \
	$(XUANTIE_GDB_DISABLE_BINUTILS_CONF_OPTS) \
	--without-included-gettext \
	--disable-werror \
	--enable-static \
	--disable-shared \
	--without-mpfr \
	--disable-source-highlight

# When only building gdbserver, we don't need zlib. But we have no way to
# tell the top-level configure that we don't need zlib: it either wants to
# build the bundled one, or use the system one.
# Since we're going to only install the gdbserver to the target, we don't
# care that the bundled zlib is built, as it is not used.
XUANTIE_GDB_CONF_OPTS += \
	--disable-gdb \
	--without-curses \
	--without-system-zlib

XUANTIE_GDB_CONF_OPTS += --enable-gdbserver
XUANTIE_GDB_DEPENDENCIES += $(TARGET_NLS_DEPENDENCIES)

# gdb 7.12+ by default builds with a C++ compiler, which doesn't work
# when we don't have C++ support in the toolchain
ifneq ($(BR2_INSTALL_LIBSTDCPP),y)
XUANTIE_GDB_CONF_OPTS += --disable-build-with-cxx
endif

# inprocess-agent can't be built statically
ifeq ($(BR2_STATIC_LIBS),y)
XUANTIE_GDB_CONF_OPTS += --disable-inprocess-agent
endif

ifeq ($(BR2_PACKAGE_EXPAT),y)
XUANTIE_GDB_CONF_OPTS += --with-expat
XUANTIE_GDB_CONF_OPTS += --with-libexpat-prefix=$(STAGING_DIR)/usr
XUANTIE_GDB_DEPENDENCIES += expat
else
XUANTIE_GDB_CONF_OPTS += --without-expat
endif

ifeq ($(BR2_PACKAGE_XZ),y)
XUANTIE_GDB_CONF_OPTS += --with-lzma
XUANTIE_GDB_CONF_OPTS += --with-liblzma-prefix=$(STAGING_DIR)/usr
XUANTIE_GDB_DEPENDENCIES += xz
else
XUANTIE_GDB_CONF_OPTS += --without-lzma
endif

ifeq ($(BR2_PACKAGE_XUANTIE_GDB_PYTHON),)
# This removes some unneeded Python scripts and XML target description
# files that are not useful for a normal usage of the debugger.
define XUANTIE_GDB_REMOVE_UNNEEDED_FILES
	$(RM) -rf $(TARGET_DIR)/usr/share/gdb
endef

XUANTIE_GDB_POST_INSTALL_TARGET_HOOKS += XUANTIE_GDB_REMOVE_UNNEEDED_FILES
endif

# This installs the gdbserver somewhere into the $(HOST_DIR) so that
# it becomes an integral part of the SDK, if the toolchain generated
# by Buildroot is later used as an external toolchain. We install it
# in debug-root/usr/bin/gdbserver so that it matches what Crosstool-NG
# does.
define XUANTIE_GDB_SDK_INSTALL_XUANTIE_GDBSERVER
	$(INSTALL) -D -m 0755 $(TARGET_DIR)/usr/bin/gdbserver \
		$(HOST_DIR)/$(GNU_TARGET_NAME)/debug-root/usr/bin/gdbserver
endef

ifeq ($(BR2_PACKAGE_XUANTIE_GDB_SERVER),y)
XUANTIE_GDB_POST_INSTALL_TARGET_HOOKS += XUANTIE_GDB_SDK_INSTALL_XUANTIE_GDBSERVER
endif

$(eval $(autotools-package))
