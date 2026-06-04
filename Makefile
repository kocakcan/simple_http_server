CC	= gcc
CFLAGS	= -Wall -Wextra -Wpedantic -g

SRCS	= main.c request.c response.c router.c handlers.c
OBJS	= $(SRCS:.c=.o)
TARGET	= http_server

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
