CC=gcc
CFLAGS =-Wall -g

SRC=$(wildcard *.c)
TARGET=$(patsubst %.c, %, $(SRC))

all: $(TARGET)

%:%.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGET)
