include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

RKVPU_VERSION=1.0
RKVPU_TARGET=rkvpu
ARCHIVE=rkvpu_depends_$(RKVPU_VERSION).tar.bz2

all: .installed-$(PLATFORM)

.installed-$(PLATFORM): install-sources
	touch $@

unpack-sources: $(TARBALLS_LOCATION)/$(ARCHIVE)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) -xf $(TARBALLS_LOCATION)/$(ARCHIVE)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	echo "Fetching rkvpu depends"
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) -Ls -f "http://www.semperpax.be/owncloud/public.php?service=files&t=eb21e45d1f833cbf58b31d9a98295bd1&download" -o $(TARBALLS_LOCATION)/$(ARCHIVE)
	#cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

install-sources: $(TARBALLS_LOCATION)/$(ARCHIVE) unpack-sources
	mkdir -p $(PREFIX)/opt
	rm -f $(PREFIX)/opt/$(RKVPU_TARGET)
	ln -s $(CURDIR)/$(PLATFORM)/$(RKVPU_TARGET) $(PREFIX)/opt/$(RKVPU_TARGET)

clean:
	rm -f $(PREFIX)/opt/$(RKVPU_TARGET)
	rm -rf .installed-$(PLATFORM) $(PLATFORM)

distclean: clean
