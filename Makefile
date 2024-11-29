CC = gcc
CFLAGS = -Wall -Werror -pedantic -std=gnu18
LOGIN = duan
SUBMITPATH = ~cs537-1/handin/$(LOGIN)/P3

all: 
	wsh

wsh: wsh.c wsh.h
	$(CC) $(CFLAGS) $< -o $@ 

run: wsh
	./$<

pack: wsh.h wsh.c Makefile README.md slipdays.txt
	tar -czvf duan.tar.gz $^

submit: pack
	cp duan.tar.gz $(SUBMITPATH)

clean:
	rm wsh

.PHONY: all