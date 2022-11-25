CFLAGS=-O3
SRC=main.c
TARGET=simpleline

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	rm $(TARGET)
