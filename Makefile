CC=gcc
CFLAGS=-lpam -lpam_misc
TARGET=supra.c

build:
	mkdir -p build
	$(CC) $(TARGET) -o build/supra $(CFLAGS)

perms: build
	sudo chown root:root build/supra
	sudo chmod 4111 build/supra

install: perms
	echo "DONT INSTALL THIS!!!"
	echo "THIS HAS MANY MAJOR SECURITY FLAWS!!!"

check: perms
ifeq ($(DEBUG),1)
	echo "1001" | sudo tee /etc/supra.allow
	printf 'auth    required pam_unix.so\naccount required pam_unix.so\n' | sudo tee /etc/pam.d/supra
endif
	build/supra 0 touch /root/test
	sudo rm -f /root/test

clean:
	sudo rm -rf build
