CC = gcc
CFLAGS = -Wall -O2
TARGET = EARTH_5G
DATA = data.dat

.PHONY: all run clean

all: $(TARGET)

$(TARGET): EARTH_5G.c
	$(CC) $(CFLAGS) -o $(TARGET) EARTH_5G.c

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(DATA)

