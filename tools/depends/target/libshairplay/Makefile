include ../../Makefile.include
DEPS= ../../Makefile.include 

# lib name, version
LIBNAME=shairplay
VERSION=139d5ef
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)


LIBDYLIB=$(PLATFORM)/src/lib/.libs/libshairplay.a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
ifeq ($(OS),ios)
	cd $(PLATFORM); patch -p1 < ../xcode-llmvfix.patch
endif
	cd $(PLATFORM); patch -p1 < ../02-fixipv4ipv6race.patch
	cd $(PLATFORM); patch -p1 < ../03-fixpasswordauthitunes.patch
	cd $(PLATFORM); $(AUTORECONF) -vif
	cd $(PLATFORM); $(CONFIGURE)

$(LIBDYLIB): $(PLATFORM)
	$(MAKE) -j 1 -C $(PLATFORM)

.installed-$(PLATFORM): $(LIBDYLIB)
	$(MAKE) -C $(PLATFORM) install
ifeq ($(OS),android)
	rm -f $(PREFIX)/lib/libshairplaylibshairplay.la $(PREFIX)/lib/libshairplay.so $(PREFIX)/lib/libshairplay.so.0
	mv -f $(PREFIX)/lib/libshairplay.so.0.0.0 $(PREFIX)/lib/libxbshairplay.so
	ln -sf $(PREFIX)/lib/libxbshairplay.so $(PREFIX)/lib/libshairplay.so
	$(RPL) -e "libshairplay.so.0" "libxbshairplay.so" $(PREFIX)/lib/libxbshairplay.so
	-$(READELF) --dynamic $(PREFIX)/lib/libshairplay.so | grep ibrary
endif

	touch $@

clean:
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)

