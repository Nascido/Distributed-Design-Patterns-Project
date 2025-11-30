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


