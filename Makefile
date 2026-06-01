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

clean: deb-clean arch-clean
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

	sudo install -o root -g root -m4111 build/tsux packaging/deb/usr/bin/tsux
	install -m644 $(BUILD)/docs/tsux.1 packaging/deb/usr/share/man/man1/tsux.1

	printf "Package: tsux\n" > packaging/deb/DEBIAN/control
	printf "Version: $(VERSION)\n" >> packaging/deb/DEBIAN/control
	printf "Section: utils\n" >> packaging/deb/DEBIAN/control
	printf "Priority: optional\n" >> packaging/deb/DEBIAN/control
	printf "Architecture: amd64\n" >> packaging/deb/DEBIAN/control
	printf "Maintainer: KitekXP\n" >> packaging/deb/DEBIAN/control
	printf "Description: Small alternative to sudo\n" >> packaging/deb/DEBIAN/control
	printf "Homepage: https://github.com/KitekXP/tsux\n" >> packaging/deb/DEBIAN/control

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
# META
# -------------------------

package: deb arch
	
