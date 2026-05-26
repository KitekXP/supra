CC=gcc
CFLAGS=-lpam -lpamc -lpam_misc
TARGET=supra.c

build:
	mkdir build
	$(CC) $(CFLAGS) $(TARGET) -o build/supra

perms: build
	sudo chown root:root build/supra
	sudo chmod 4111 build/supra

install: perms
	echo "DONT INSTALL THIS!!!"
	echo "THIS HAS MANY MAJOR SECURITY FLAWS!!!"

check: perms
    sudo echo "1001" >> /etc/supra.allow
    sudo echo "auth    required pam_unix.so" >> /etc/pam.d/supra
    sudo echo "account required pam_unix.so" >> /etc/pam.d/supra
    build/supra 0 touch /root/test
    sudo rm -f /root/test

clean:
	sudo rm -rf build
