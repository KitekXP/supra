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

clean:
	sudo rm -rf build
