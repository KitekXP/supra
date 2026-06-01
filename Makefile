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
        deb arch package

all: build

# -------------------------
# BUILD
# -------------------------

build:
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) src/tsux.c -o $(BIN) $(LDFLAGS) $(LDLIBS)

run: build
	$(BIN)

clean:
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
	rm -rf $(BUILD)/deb
	mkdir -p $(BUILD)/deb/DEBIAN
	mkdir -p $(BUILD)/deb/usr/bin
	mkdir -p $(BUILD)/deb/usr/share/man/man1

	install -m755 $(BIN) $(BUILD)/deb/usr/bin/$(NAME)
	install -m644 $(BUILD)/docs/tsux.1 $(BUILD)/deb/usr/share/man/man1/tsux.1

	printf "Package: tsux\n" > $(BUILD)/deb/DEBIAN/control
	printf "Version: $(VERSION)\n" >> $(BUILD)/deb/DEBIAN/control
	printf "Section: utils\n" >> $(BUILD)/deb/DEBIAN/control
	printf "Priority: optional\n" >> $(BUILD)/deb/DEBIAN/control
	printf "Architecture: amd64\n" >> $(BUILD)/deb/DEBIAN/control
	printf "Maintainer: KitekXP\n" >> $(BUILD)/deb/DEBIAN/control
	printf "Description: Small alternative to sudo\n" >> $(BUILD)/deb/DEBIAN/control
	printf "Homepage: https://github.com/KitekXP/tsux\n" >> $(BUILD)/deb/DEBIAN/control

	dpkg-deb --build $(BUILD)/deb tsux_$(VERSION)_amd64.deb

# -------------------------
# ARCH PACKAGE
# -------------------------

arch: build docs
	rm -rf $(BUILD)/arch
	mkdir -p $(BUILD)/arch/usr/bin
	mkdir -p $(BUILD)/arch/usr/share/man/man1

	install -m755 $(BIN) $(BUILD)/arch/usr/bin/$(NAME)
	install -m644 $(BUILD)/docs/tsux.1 $(BUILD)/arch/usr/share/man/man1/tsux.1

	# metadata for pacman
	echo "pkgname = $(NAME)" > $(BUILD)/arch/.PKGINFO
	echo "pkgver = $(VERSION)" >> $(BUILD)/arch/.PKGINFO
	echo "pkgdesc = Small alternative to sudo" >> $(BUILD)/arch/.PKGINFO
	echo "url = https://github.com/KitekXP/tsux" >> $(BUILD)/arch/.PKGINFO
	echo "builddate = $(shell date +%s)" >> $(BUILD)/arch/.PKGINFO
	echo "packager = KitekXP" >> $(BUILD)/arch/.PKGINFO
	echo "arch = x86_64" >> $(BUILD)/arch/.PKGINFO
	echo "license = GPL-3.0" >> $(BUILD)/arch/.PKGINFO
	echo "depend = pam" >> $(BUILD)/arch/.PKGINFO
	echo "depend = pam_misc" >> $(BUILD)/arch/.PKGINFO

	cd $(BUILD)/arch && \
	tar --zstd -cf ../../tsux-$(VERSION)-x86_64.pkg.tar.zst .PKGINFO usr

# -------------------------
# META
# -------------------------

package: deb arch
