CC=gcc
CFLAGS=-Oz -s -std=gnu11
LDFLAGS=-Wl,--gc-sections -lpam -lpam_misc
TARGET=tsux.c
SHELL_EXT=sh # HAS TO BE THE EXTENSION FOR SHELL SCRIPTS

build:
	mkdir -p build
	$(CC) $(CFLAGS) $(TARGET) -o build/tsux $(LDFLAGS)

perms: build
# Gives the permissions required for using setuid() and setgid()
	su -c 'chown root:root build/tsux'
	su -c 'chmod 4111 build/tsux'

builddocs:
	pandoc -s -t man docs/tsux.md -o build/docs/tsux.1

installdocs:
	su -c 'cp build/docs/tsux.1 /usr/share/man/man1/'

install: perms installdocs
# Warns the user
	echo "I do not recommend installing, but you can use DANGER=1 to install it"
# If you really want to you still can
ifeq ($(DANGER),1)
	su -c 'touch /etc/tsux.allow'
	su -c 'echo "auth    required pam_unix.so"'
	su -c 'echo "account required pam_unix.so"'
	su -c 'cp build/tsux /bin/tsux'
	su -c 'chmod 4111 /bin/tsux'
endif

check: perms
# Checks if the program works by creating a file in /root/ (UNSAFE)
	build/tsux 0 tests/test.$(SHELL_EXT)
	su -c 'cat /root/test'

cleandocs:
	rm -rf build/docs

cleanchecks: clean
# Cleans up the checks
	su -c 'rm -rf /root/test'

clean: cleandocs
# Cleans up only the build
	su -c 'rm -rf build'
