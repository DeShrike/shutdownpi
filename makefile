CC=gcc
AR=ar

all: shutdownpi

rmbin:
	rm shutdownpi

shutdownpi: shutdownpi.o utils.o http.o ini.o config.o
	$(CC) -o shutdownpi shutdownpi.o utils.o ini.o config.o http.o -l wiringPi

shutdownpi.o: shutdownpi.c utils.h http.h config.h shutdownpi.h
	$(CC) -c -Wall -O3 shutdownpi.c

utils.o: utils.c utils.h
	$(CC) -c -Wall -O3 utils.c

http.o: http.c http.h
	$(CC) -c -Wall -O3 http.c

config.o: config.c config.h utils.h shutdownpi.h
	$(CC) -c -Wall -O3 config.c

ini.o: ini.c ini.h
	$(CC) -c -Wall -O3 ini.c

clean:
	rm *.o

install: shutdownpi shutdownpi.service
	sudo cp shutdownpi /usr/bin
	sudo cp shutdownpi.service /etc/systemd/system
	sudo systemctl daemon-reload

start:
	sudo systemctl start shutdownpi

stop:
	sudo systemctl stop shutdownpi

enable:
	sudo systemctl enable shutdownpi

disable:
	sudo systemctl disable shutdownpi
