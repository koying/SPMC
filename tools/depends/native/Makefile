include ../Makefile.include

ifneq ($(shell test -f $(NATIVEPREFIX)/share/config.site && echo 1),1)
  $(error Error: $(NATIVEPREFIX)/share/config.site  is missing. Please reconfigure depends to generate it)
endif

NATIVE = rpl-native libsquish-native JsonSchemaBuilder TexturePacker google-breakpad-native \
         swig-native python26-native
# NATIVE= m4-native gettext-native autoconf-native automake-native \
        libtool-native pkg-config-native yasm-native cmake-native \
        gas-preprocessor-native python26-native zlib-native \
        pcre-native swig-native rpl-native libsquish-native \
        JsonSchemaBuilder TexturePacker \
        google-breakpad-native


ifeq ($(OS),ios)
  NATIVE += dpkg-native xz-native tar-native fakeroot-native gen_entitlements-native ldid-native
endif

ifeq ($(OS),osx)
  NATIVE += xz-native tar-native
endif

.PHONY: $(NATIVE) native

all: native
	@echo "Dependencies built successfully."

# Dependency layout for parallel builds
TexturePacker: libsquish-native

native: $(NATIVE)
$(NATIVE):
	$(MAKE) -C $@
clean:
	for d in $(NATIVE); do $(MAKE) -C $$d clean; done

# Debug target, this will DELETE all data in staging!
test-dependencies:
	( for d in $(NATIVE); do \
	rm -rf $(NATIVEPREFIX); \
        mkdir -p $(NATIVEPREFIX)/include $(NATIVEPREFIX)/share $(NATIVEPREFIX)/bin; \
	cp -f config.site $(NATIVEPREFIX)/share/; \
	$(MAKE) distclean; \
	$(MAKE) $$d; done ) && echo "$@ built successfully"

distclean::
	for d in $(NATIVE); do $(MAKE) -C $$d distclean; done

