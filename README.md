
# Trabalho 2 de Programação Paralela e Distribuída

> **Disciplina:** INE5645 - Programação Paralela e Distribuída (UFSC)  
> **Semestre:** 2025/2

## Sobre o Projeto
Este repositório contém a implementação da infraestrutura central de um sistema de cotações financeiras em tempo real. O projeto foi desenvolvido com o objetivo acadêmico de explorar e aplicar **Padrões de Projeto para Sistemas Distribuídos** (Distributed Design Patterns) para mitigar desafios de escalabilidade, disponibilidade e latência.

Desenvolvido em **C** utilizando **Sockets TCP/IP** e **I/O Multiplexing**, o sistema simula um ambiente de alta performance resiliente a falhas, implementando manualmente os seguintes padrões arquiteturais:

* **Circuit Breaker:** Proteção contra falhas em cascata e latência de serviços externos.
* **Pub/Sub (Publisher/Subscriber):** Difusão assíncrona de atualizações de preços para múltiplos clientes.
* **Sharding (Particionamento):** Distribuição de persistência de dados baseada em chaves (moedas) para balanceamento de carga.
* **Scatter/Gather:** Processamento paralelo de consultas complexas e agregação de resultados.

## cripto_server.c
Serviço externo que representa uma Bolsa de Valores ou API financeira. Mantém as cotações reais e pode apresentar intermitência (latência, falhas ou manutenção). O `primary_server` consulta este serviço para obter preços em tempo real.

## primary_server.c
Serviço central que recebe atualizações do `cripto_server`, persiste dados em shards e atende clientes finais. Principais responsabilidades:

- Publicar atualizações por tópico (Pub/Sub) e gerenciar assinaturas de clientes.
- Roteamento e persistência em shards por criptomoeda (ex.: BTC, ETH).
- Responder a consultas: `media_historica` (agrega dados de todos os shards) e `cotacao_atual(X)` (consulta o serviço externo ou usa histórico como fallback).

### Circuit Breaker

O padrão *Circuit Breaker* protege o `primary_server` de chamadas repetidas a um serviço externo instável (`cripto_server`) evitando gasto desnecessário de recursos e melhorando a latência percebida pelos clientes.

- **Como funciona aqui:** Antes de cada consulta ao mock externo o código verifica `cb_allow_request(&cb_mock)`. Se o circuito estiver aberto a chamada não é tentada (fail-fast) e a função retorna um erro imediato. Chamadas que falham acionam `cb_record_failure(&cb_mock)`; chamadas bem-sucedidas acionam `cb_record_success(&cb_mock)`, que restauram o circuito.
- **Parâmetros usados:** neste projeto o circuito é inicializado com `cb_init(&cb_mock, 3, 10)`, ou seja, 3 falhas consecutivas abrem o circuito e ele fica aberto por 10 segundos antes de permitir nova tentativa.
- **Benefício prático:** reduz tentativas redundantes a serviços fora do ar, protege recursos locais e fornece resposta rápida (erro imediato) ao cliente quando a dependência está indisponível.


### Pub/Sub
Ao conectar-se, o cliente é inscrito por padrão para receber todas as cotações; pode alterar a assinatura com:

```
SUBSCRIBE X
```

Onde `X` pode ser `BTC`, `ETH` ou `TODOS`.

### Sharding + Scatter/Gather
As atualizações são particionadas por criptomoeda em shards distintos (ex.: um shard para `BTC`, outro para `ETH`).

- **Armazenamento:** o `primary_server` roteia gravações para o shard apropriado com base na chave (criptomoeda).
- **Consulta `media_historica`:** realiza consultas paralelas (scatter) aos shards e agrega (gather) os resultados para calcular médias.
- **Fallback `cotacao_atual`:** se o serviço externo falhar, o `primary_server` consulta apenas o shard da moeda requisitada para responder.


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

