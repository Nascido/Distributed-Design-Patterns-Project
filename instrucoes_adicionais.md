## Como executar o projeto

- Pré-requisitos: gcc, make, `gnome-terminal` (opcional) e `nc` (netcat).

- Compilar tudo:

```
make clean && make
```

- Executar automaticamente (abre terminais):

```
./scripts/execute.sh
```

- Executar manualmente (cada comando em um terminal separado):

```
./bin/db_server.run 9801
./bin/db_server.run 9802
./bin/cripto_server.run
./bin/primary_server.run
```

## Ajustando latência e taxa de erros do mock

Edite `src/external/cripto_server.c` e modifique as constantes:

```c
#define CHANCE_FALHA 20    // porcentagem de falha (ex: 20 = 20%)
#define MAX_LATENCIA 1     // latência máxima simulada (segundos)
```

Depois de alterar, recompile o mock e reinicie-o:

```
make external
pkill -f cripto_server.run
./bin/cripto_server.run
```

Testes rápidos:

```
echo -n "BTC" | nc 127.0.0.1 8080    # consulta direta ao mock
nc 127.0.0.1 9090                   # conecta ao primary (interativo)
```

## Comandos interativos (usando nc no primary server)

Conecte-se ao servidor primário com:

```bash
nc 127.0.0.1 9090
```

Comandos disponíveis (digite e pressione Enter):

- SUBSCRIBE BTC
	- Exemplo: `SUBSCRIBE BTC`
	- O servidor responde confirmando a assinatura. A partir daí o cliente receberá atualizações apenas para BTC.
	- Resposta esperada: `OK: Assinado no topico BTC` (ou mensagem de confirmação similar).

- SUBSCRIBE ETH
	- Exemplo: `SUBSCRIBE ETH`
	- Assina o cliente para receber somente atualizações de ETH.
	- Resposta esperada: `OK: Assinado no topico ETH`.

- SUBSCRIBE TODOS
	- Exemplo: `SUBSCRIBE TODOS`
	- Cancela o filtro e passa a receber todas as cotações publicadas.

- media_historica
	- Exemplo: `media_historica`
	- Dispara uma operação scatter/gather aos shards que retorna um relatório agregado.
	- Resposta esperada (exemplo):

```
=== RELATORIO DE MERCADO ===
Media BTC (Ultimos 10): 59900.23
Media ETH (Ultimos 10): 3010.45
Tendencia: Normal
```

- cotacao_atual(XYZ)
	- Exemplo: `cotacao_atual(BTC)` ou `cotacao_atual(ETH)` (substitua XYZ pela moeda desejada)
	- O servidor tentará consultar o mock externo (via Circuit Breaker). Se o mock estiver disponível retorna o preço atual. Caso contrário, faz fallback para o dado mais recente no shard.
	- Possíveis respostas:
		- `BTC: 60234.56`  (preço atual retornado pelo mock)
		- `BTC: 60012.34 (Cache - Servico Off)`  (fallback para cache quando mock indisponível)
		- `Erro: Servico Indisponivel e sem dados historicos para 'XYZ'`  (sem dados de fallback)

Dicas rápidas:
- Envie `SUBSCRIBE TODOS` para receber todos os alerts publicados.
- Depois de assinar um tópico, aguarde as mensagens de publish do servidor (elas chegam automaticamente quando o ciclo de monitoramento atualiza os preços).
- Para sair do `nc` use Ctrl+C ou Ctrl+D dependendo da sua sessão.

