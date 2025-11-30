#!/bin/bash

# Definições de cores para melhor visualização no terminal
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Iniciando Infraestrutura do Sistema em Terminais ===${NC}"

# --- PASSO 1: COMPILAÇÃO ---
echo -e "${BLUE}--- 1. Compilando o Projeto (make) ---${NC}"
make clean
make
if [ $? -ne 0 ]; then
    echo -e "${RED}Erro na compilação. Verifique os erros acima.${NC}"
    exit 1
fi
echo -e "${GREEN}Compilação concluída com sucesso!${NC}"

# --- PASSO 2: LIMPEZA DE PROCESSOS ANTIGOS ---
echo -e "${BLUE}--- 2. Matando processos antigos... ---${NC}"
pkill -f cripto_server.run
pkill -f primary_server.run
pkill -f db_server.run
sleep 1

# --- PASSO 3: LANÇANDO SERVIDORES EM NOVOS TERMINAIS ---

# Verifica se o gnome-terminal está disponível
if ! command -v gnome-terminal &> /dev/null; then
    echo -e "${RED}Erro: gnome-terminal não encontrado.${NC}"
    echo "Instale-o ou rode os binários manualmente."
    exit 1
fi

echo -e "${BLUE}Abrindo 4 terminais para a infraestrutura...${NC}"

# 1. Shard BTC (Porta 9801) - Canto Superior Esquerdo
# Usa 'exec bash' no final para manter o terminal aberto se o servidor cair
gnome-terminal --title="Shard 1 (BTC - 9801)" --geometry=70x15+0+0 -- bash -c "echo 'Iniciando DB Shard 1...'; ./bin/db_server.run 9801; echo 'Processo terminou.'; exec bash"

# 2. Shard ETH (Porta 9802) - Canto Inferior Esquerdo
gnome-terminal --title="Shard 2 (ETH - 9802)" --geometry=70x15+0+350 -- bash -c "echo 'Iniciando DB Shard 2...'; ./bin/db_server.run 9802; echo 'Processo terminou.'; exec bash"

sleep 2

# 3. Mock da Bolsa (Porta 8080) - Canto Superior Direito
gnome-terminal --title="Mock Bolsa (8080)" --geometry=70x15+750+0 -- bash -c "echo 'Iniciando Mock Server...'; ./bin/cripto_server.run; echo 'Processo terminou.'; exec bash"

sleep 1 

# 4. Servidor Primário (Porta 9090) - Canto Inferior Direito
gnome-terminal --title="PRIMARY SERVER (HUB - 9090)" --geometry=70x15+750+350 -- bash -c "echo 'Aguardando dependências...'; sleep 1; echo 'Iniciando Primary Server...'; ./bin/primary_server.run; echo 'Processo terminou.'; exec bash"


echo -e "${BLUE}Abrindo 4 terminais para clientes...${NC}"

sleep 2

# 5. Cliente
gnome-terminal --title="Cliente 1" --geometry=70x15+750+0 -- bash -c "echo 'Cliente ...'; nc 127.0.0.1 9090 ; echo 'Processo terminou.'; exec bash"
gnome-terminal --title="Cliente 2" --geometry=70x15+750+0 -- bash -c "echo 'Cliente ...'; nc 127.0.0.1 9090 ; echo 'Processo terminou.'; exec bash"
gnome-terminal --title="Cliente 3" --geometry=70x15+750+0 -- bash -c "echo 'Cliente ...'; nc 127.0.0.1 9090 ; echo 'Processo terminou.'; exec bash"
gnome-terminal --title="Cliente 4" --geometry=70x15+750+0 -- bash -c "echo 'Cliente ...'; nc 127.0.0.1 9090 ; echo 'Processo terminou.'; exec bash"


echo -e "${GREEN}Todos os terminais foram lançados!${NC}"
echo -e "Para fechar tudo depois, execute: ${RED}pkill -f .run${NC}"