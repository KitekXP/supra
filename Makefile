CC=gcc
CFLAGS=-lpam -lpamc -lpam_misc
TARGET=supra.c

build:
	mkdir build
	$(CC) $(TARGET) -o build/supra $(CFLAGS)

perms: build
	sudo chown root:root build/supra
	sudo chmod 4111 build/supra

install: perms
	echo "DONT INSTALL THIS!!!"
	echo "THIS HAS MANY MAJOR SECURITY FLAWS!!!"

check: perms
	echo "1001" | sudo tee -a /etc/supra.allow
    printf 'auth    required pam_unix.so\naccount required pam_unix.so\n' | sudo tee /etc/pam.d/supra
	build/supra 0 touch /root/test
	sudo rm -f /root/test

clean:
	sudo rm -rf build
