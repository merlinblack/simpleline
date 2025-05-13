CFLAGS=-O3 -Wall -Werror
SRC=main.c
TARGET=simpleline
SETUP_SCRIPT=$(TARGET)_setup
INSTALL_PREFIX=$(HOME)/bin

.PHONY: all
all: $(TARGET) $(SETUP_SCRIPT) tags

.PHONY: debug
debug: CFLAGS += -fsanitize=address -fsanitize=undefined
debug: all

.PHONY: clean
clean:
	rm $(TARGET)
	rm $(SETUP_SCRIPT)
	rm tags

.PHONY: install
install: all
	mkdir -p $(INSTALL_PREFIX)
	install $(TARGET) $(INSTALL_PREFIX)
	install $(SETUP_SCRIPT) $(INSTALL_PREFIX)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

$(SETUP_SCRIPT): setup.in
	sed "s#INSTALL_PREFIX#$(abspath $(INSTALL_PREFIX))#;s#TARGET#$(TARGET)#" $< > $@

.ONESHELL:
tags:	$(SRC)
	@if command -V ctags 2>&1 > /dev/null; then
		ctags $(SRC)
	else
		touch tags
	fi
