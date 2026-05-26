CC=gcc
CFLAGS=-lpam -lpam_misc
TARGET=tsux.c
SHELL=/bin/sh

build:
	mkdir -p build
	$(CC) $(TARGET) -o build/tsux $(CFLAGS)

perms: build
	su -c 'chown root:root build/tsux'
	su -c 'chmod 4111 build/tsux'

install: perms
	echo "DONT INSTALL THIS!!!"
	echo "THIS HAS MANY MAJOR SECURITY FLAWS!!!"
ifeq ($(DANGER),1)
	echo "DANGER OPTION = 1, INSTALLING!!!"
	su -c 'touch /etc/tsux.allow'
	su -c 'echo "auth    required pam_unix.so"'
	su -c 'echo "account required pam_unix.so"'
	su -c 'cp build/tsux /bin/tsux'
endif

check: perms
	tests/test.$(SHELL)
	cat /root/test

cleanchecks:
	su -c 'rm -rf /root/test'

clean: cleanchecks
	su -c 'rm -rf build'
