CC=gcc

CFLAGS=-O2 -std=gnu11 -Wall -Wextra
LDFLAGS=-Wl,--gc-sections
LDLIBS=-lpam -lpam_misc

NAME=tsux
VERSION=1.0.0

BUILD=build
BIN=$(BUILD)/$(NAME)

PREFIX?=/usr/local

# -------------------------
# DEFAULT
# -------------------------

.PHONY: all build clean run docs install uninstall install-docs \
        deb arch package arch-clean deb-clean

all: build

# -------------------------
# HELP
# -------------------------

help:
	@echo "tsux build system"
	@echo ""
	@echo "Targets:"
	@echo "  make build          Build binary"
	@echo "  make run            Build and run"
	@echo "  make clean          Remove all build artifacts"
	@echo "  make docs           Generate man page"
	@echo ""
	@echo "  make install        Install to $(PREFIX)"
	@echo "  make uninstall      Remove installed files"
	@echo "  make install-docs   Install man page"
	@echo ""
	@echo "  make deb            Build Debian package"
	@echo "  make arch           Build Arch package"
	@echo "  make package        Build both deb and arch"
	@echo ""
	@echo "  make deb-clean      Clean Debian packaging"
	@echo "  make arch-clean     Clean Arch packaging"
	@echo ""
	@echo "Variables:"
	@echo "  PREFIX=$(PREFIX)"
	@echo "  CC=$(CC)"
	@echo "  VERSION=$(VERSION)"

# -------------------------
# BUILD
# -------------------------

build:
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) src/tsux.c -o $(BIN) $(LDFLAGS) $(LDLIBS)

run: build
	$(BIN)

deb-clean:
	rm -rf packaging/deb/*

arch-clean:
	rm -rf packaging/arch/tsux \
		      packaging/arch/tsux.1 \
		      packaging/arch/COPYING
	rm -rf packaging/arch/pkg
	rm -rf packaging/arch/src
	rm -rf packaging/arch/tsux-$(VERSION)-1-x86_64.pkg.tar.zst

okiz-clean:
	rm -rf package/files
	rm -rf tsux.tar.zst

clean: deb-clean arch-clean okiz-clean
	rm -rf $(BUILD)

# -------------------------
# DOCS
# -------------------------

docs:
	mkdir -p $(BUILD)/docs
	pandoc -s -t man docs/tsux.md -o $(BUILD)/docs/tsux.1

# -------------------------
# INSTALL (SAFE ONLY)
# -------------------------

install: build
	install -Dm755 $(BIN) $(PREFIX)/bin/$(NAME)

install-docs: docs
	install -Dm644 $(BUILD)/docs/tsux.1 $(PREFIX)/share/man/man1/tsux.1

uninstall:
	rm -f $(PREFIX)/bin/$(NAME)
	rm -f $(PREFIX)/share/man/man1/tsux.1

# -------------------------
# DEBIAN PACKAGE
# -------------------------

deb: build docs
	mkdir -p packaging/deb/DEBIAN
	mkdir -p packaging/deb/usr/bin
	mkdir -p packaging/deb/usr/share/man/man1

	sudo install -m755 build/tsux packaging/deb/usr/bin/tsux
	install -m644 $(BUILD)/docs/tsux.1 packaging/deb/usr/share/man/man1/tsux.1

	printf "Package: tsux\n" > packaging/deb/DEBIAN/control
	printf "Version: $(VERSION)\n" >> packaging/deb/DEBIAN/control
	printf "Section: utils\n" >> packaging/deb/DEBIAN/control
	printf "Priority: optional\n" >> packaging/deb/DEBIAN/control
	printf "Architecture: amd64\n" >> packaging/deb/DEBIAN/control
	printf "Maintainer: KitekXP\n" >> packaging/deb/DEBIAN/control
	printf "Description: Small alternative to sudo\n" >> packaging/deb/DEBIAN/control
	printf "Homepage: https://github.com/KitekXP/tsux\n" >> packaging/deb/DEBIAN/control

	printf "#!/bin/sh\nset -e\n\nchmod 4111 /usr/bin/tsux\n\nexit 0\n" >> packaging/deb/DEBIAN/postinst
	chmod 755 packaging/deb/DEBIAN/postinst

	dpkg-deb --build packaging/deb packaging/deb/tsux_$(VERSION)_amd64.deb

# -------------------------
# ARCH PACKAGE
# -------------------------

arch: build docs
	rm -f packaging/arch/tsux \
	      packaging/arch/tsux.1 \
	      packaging/arch/COPYING

	cp build/tsux packaging/arch/
	cp build/docs/tsux.1 packaging/arch/
	cp COPYING packaging/arch/

	cd packaging/arch && makepkg -f
	cd ../..

# -------------------------
# OKIZ
# -------------------------

okiz: build docs
	mkdir --parent package/files/usr/bin
	mkdir --parent package/files/usr/share/man/man1

	sudo install -m=4111 -o=root -g=root build/tsux package/files/usr/bin/tsux
	install -m=644 build/docs/tsux.1 package/files/usr/share/man/man1/tsux.1

# -------------------------
# META
# -------------------------

package: deb arch
	
