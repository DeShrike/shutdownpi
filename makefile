CC=gcc
AR=ar
LIBS=-lwiringPi
CFLAGS=-Wall -O3
OBJ=shutdownpi.o utils.o http.o ini.o config.o

all: shutdownpi

rmbin:
	rm shutdownpi

shutdownpi: $(OBJ)
	$(CC) -o shutdownpi $(OBJ) $(LIBS)

shutdownpi.o: shutdownpi.c utils.h http.h config.h shutdownpi.h
	$(CC) -c $(CFLAGS) shutdownpi.c

utils.o: utils.c utils.h
	$(CC) -c $(CFLAGS) utils.c

http.o: http.c http.h
	$(CC) -c $(CFLAGS) http.c

config.o: config.c config.h utils.h shutdownpi.h
	$(CC) -c $(CFLAGS) config.c

ini.o: ini.c ini.h
	$(CC) -c $(CFLAGS) ini.c

clean:
	rm *.o

install: shutdownpi shutdownpi.service
	sudo cp shutdownpi /usr/bin
	sudo cp shutdownpi.service /etc/systemd/system
	sudo cp shutdownpi.ini /etc
	sudo systemctl daemon-reload

uninstall: shutdownpi shutdownpi.service
	sudo systemctl stop shutdownpi
	sudo systemctl disable shutdownpi
	sudo rm /usr/bin/shutdownpi
	sudo rm /etc/systemd/system/shutdownpi.service
	sudo rm /etc/shutdownpi.ini
	sudo systemctl daemon-reload

start:
	sudo systemctl start shutdownpi

stop:
	sudo systemctl stop shutdownpi

enable:
	sudo systemctl enable shutdownpi

disable:
	sudo systemctl disable shutdownpi
