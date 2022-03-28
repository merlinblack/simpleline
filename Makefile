CFLAGS=-O3
SRC=main.c
TARGET=simpleline

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

.PHONY: clean
clean:
	rm $(TARGET)
