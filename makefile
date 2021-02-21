CC=gcc
AR=ar

all: shutdownpi

rmbin:
	rm shutdownpi

shutdownpi: shutdownpi.o
	$(CC) -o shutdownpi shutdownpi.o -l wiringPi

shutdownpi.o: shutdownpi.c
	$(CC) -c -O3 shutdownpi.c

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
