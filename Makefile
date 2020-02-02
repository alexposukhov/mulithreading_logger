
all:
	gcc main.c  loglib_api.c log.c -o syslogtest -lpthread -Wall -lz
