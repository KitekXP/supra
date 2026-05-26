CC=gcc
CFLAGS=-lpam -lpam_misc
TARGET=tsux.c

build:
	mkdir -p build
	$(CC) $(TARGET) -o build/tsux $(CFLAGS)

perms: build
	sudo chown root:root build/tsux
	sudo chmod 4111 build/tsux

install: perms
	echo "DONT INSTALL THIS!!!"
	echo "THIS HAS MANY MAJOR SECURITY FLAWS!!!"

check: perms
ifeq ($(DEBUG),1)
	echo "1001" | sudo tee /etc/tsux.allow
	printf 'auth    required pam_unix.so\naccount required pam_unix.so\n' | sudo tee /etc/pam.d/tsux
endif
	build/supra 0 touch /root/test
	sudo rm -f /root/test

clean:
	sudo rm -rf build
