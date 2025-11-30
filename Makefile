# Makefile

# Compilador e Flags
CC = gcc
CFLAGS = -Wall -Wextra -g -I./src/primary 

# Diretórios
SRC_DIR = src
BIN_DIR = bin

# Criar diretório bin se não existir
$(shell mkdir -p $(BIN_DIR))

# --- Targets (Alvos de Compilação) ---

all: external primary shard

# 1. Compila o Servidor Externo (Mock) -> cripto_server.run
external: $(SRC_DIR)/external/cripto_server.c
	$(CC) $(CFLAGS) $< -o $(BIN_DIR)/cripto_server.run

# 2. Compila o Serviço Primário -> servico_primario.run
primary: $(SRC_DIR)/services/primary_server.c $(SRC_DIR)/services/circuit_breaker.c
	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/primary_server.run

# 3. Compila o Shard (Banco de Dados) -> db_server.run
shard: $(SRC_DIR)/shards/db_server.c
	$(CC) $(CFLAGS) $< -o $(BIN_DIR)/db_server.run

# Limpeza
clean:
	rm -rf $(BIN_DIR)/*.run