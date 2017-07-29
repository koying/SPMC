include ../../Makefile.include
DEPS= ../../Makefile.include Makefile samba_android_google.patch

PYTHON=python

# lib name, version
LIBNAME=samba
VERSION=4.5.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.gz
CONFIGURE= cp -f $(CONFIG_SUB) $(CONFIG_GUESS) .; \
          ./configure --prefix=$(PREFIX) \
          --hostcc=$(shell which gcc) \
          --without-cluster-support --without-ldap \
          --without-pam --enable-fhs \
          --without-winbind --disable-cups --without-ads \
          --disable-avahi \
          --without-dnsupdate \
          --with-libiconv=$(PREFIX) \
          --without-acl-support --without-ad-dc \
          --disable-tdb-mutex-locking \
          --cross-compile --cross-answers=build_answers \
          --with-static-modules=ALL \
          --builtin-libraries=replace,ccan,samba-cluster-support,smbconf,smbregistry,secrets3 \
          --bundled-libraries=talloc,tdb,pytdb,ldb,pyldb,tevent,pytevent

ifeq ($(OS),android)
CWD=$(shell pwd)/$(PLATFORM)
export CC:="$(PYTHON) $(CWD)/cc_shim.py $(CC)"
export DROIDCFLAGS:=$(CFLAGS) -O2 -D_FORTIFY_SOURCE=2 -fstack-protector-all -fPIE -Wa,--noexecstack -Wformat -Wformat-security
export LDFLAGS+=-Wl,-z,relro,-z,now
endif

LIBDYLIB=$(PLATFORM)/source3/bin/libsmbclient.a

CLEAN_FILES=$(ARCHIVE) $(PLATFORM)

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) https://download.samba.org/pub/samba/stable/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
ifeq ($(OS),android)
	cd $(PLATFORM); patch -p1 < ../samba_android_google.patch
endif
	@echo '$(CC)'
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM) 
	cd $(PLATFORM); WAF_MAKE=1 $(PYTHON) $(CWD)/buildtools/bin/waf build --targets=smbclient

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM); WAF_MAKE=1 $(PYTHON) $(CWD)/buildtools/bin/waf install --targets=smbclient
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

