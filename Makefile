CC = gcc
CFLAGS = -Wall -Wextra -O2

BIN_DIR = bin
EXT_BIN_DIR = $(BIN_DIR)/externo

SRC_DIR = src
EXT_SRC_DIR = $(SRC_DIR)/externo

TARGETS = $(BIN_DIR)/agregador $(BIN_DIR)/circuit_breaker \
		  $(EXT_BIN_DIR)/servidor_externo $(EXT_BIN_DIR)/clientes

.PHONY: all clean

all: $(TARGETS)

$(BIN_DIR)/%: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(EXT_BIN_DIR)/%: $(EXT_SRC_DIR)/%.c | $(EXT_BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(EXT_BIN_DIR):
	mkdir -p $(EXT_BIN_DIR)

clean:
	rm -rf $(BIN_DIR)
