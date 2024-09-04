CFLAGS=-O3
SRC=main.c
TARGET=simpleline
SETUP_SCRIPT=$(TARGET)_setup
INSTALL_PREFIX=$(HOME)/bin

.PHONY: all
all: $(TARGET) $(SETUP_SCRIPT)

.PHONY: clean
clean:
	rm $(TARGET)
	rm $(SETUP_SCRIPT)

.PHONY: install
install: all
	install $(TARGET) $(INSTALL_PREFIX)
	install $(SETUP_SCRIPT) $(INSTALL_PREFIX)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

$(SETUP_SCRIPT): setup.in
	sed "s#INSTALL_PREFIX#$(INSTALL_PREFIX)#" $< > $@
