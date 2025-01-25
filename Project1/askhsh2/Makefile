CC = gcc
CFLAGS = -Wall -Werror -Wextra
LDFLAGS = -lm


all:
	$(CC) launch.c passenger.c -pthread -g -o output/launch  $(CFLAGS) $(LDFLAGS) && ./output/launch

debug:
	$(CC) launch.c passenger.c -pthread -g -o output/launch $(CFLAGS) $(LDFLAGS) && ./output/launch


clean:
	rm -rf output