## README do Projeto: Fundamentação Arquitetural para o Sistema de Cotações em Tempo Real (Trabalho 2)

**Disciplina:** INE 5645 – Programação Paralela e Distribuída, Semestre 2025/2
**Objetivo do Projeto:** Proposição, justificação e implementação de padrões de projeto para mitigar desafios de sistemas distribuídos.

---

### 1. Contexto e Objetivo Arquitetural

O Trabalho 2 consiste na construção da infraestrutura central de um **Sistema de Cotações em Tempo Real**. Este sistema deve ser projetado para alta escalabilidade e disponibilidade, suportando milhões de usuários e gerenciando um volume massivo de dados históricos de transações. A aplicação de padrões de projeto distribuídos é essencial para garantir a comunicação eficiente, a cooperação entre módulos e o desempenho do sistema.

### 2. Desafios Arquiteturais a Serem Abordados

A arquitetura do Sistema de Cotações enfrenta quatro desafios críticos que requerem a aplicação de padrões distribuídos:

1.  **Escalabilidade do Repositório de Dados Históricos:** O banco de dados de histórico de transações está se tornando excessivamente grande e lento, o que impõe a necessidade de particionar e distribuir esses dados para gerenciar o custo de armazenamento e a latência das consultas.
2.  **Distribuição de Eventos em Tempo Real:** Clientes precisam ser notificados instantaneamente sobre mudanças de preço (eventos), porém, o serviço de cotação não deve ser responsável por conhecer ou gerenciar ativamente cada conexão de cliente (desacoplamento).
3.  **Tolerância a Falhas de Dependências Externas:** O motor de cotações primário possui dependência de um serviço externo. A falha ou lentidão deste serviço não pode causar a indisponibilidade ou falha em cascata de todo o sistema principal.
4.  **Agregação de Consultas Complexas:** O sistema deve ter a capacidade de responder rapidamente a consultas complexas que combinam múltiplos tipos de dados (e.g., preço médio atual de dados replicados em tempo real e as últimas 10 transações históricas de dados particionados), sendo necessário combinar essas informações parciais em uma única resposta unificada.

### 3. Padrões de Projeto Distribuídos Requeridos

O projeto deve focar na implementação de, pelo menos, três dos seguintes padrões, conforme especificado para a solução dos desafios:

| Módulo/Componente | Padrão de Projeto | Problema Resolvido e Aplicação | Fonte |
| :--- | :--- | :--- | :--- |
| **Serviço de Cotações em Tempo Real** | **Circuit Breaker (Disjuntor)** | Protege o serviço primário contra falhas do serviço externo. O padrão resolve o problema de recursos críticos ficarem ocupados esperando *time-outs* repetitivos durante falhas de conexão, prevenindo falhas em cascata e melhorando a disponibilidade. | INE5645\_aula15 |
| **Distribuição de Preços** | **Publisher/Subscriber (Pub/Sub)** | Garante a notificação instantânea de mudanças de preço. O padrão utiliza um *Broker* para gerenciar e difundir eventos de novas cotações (mensagens associadas a um tópico) de forma assíncrona, difundindo informações para componentes que não são conhecidos ativamente pelo publicador. | INE5645\_aula15 |
| **Serviço de Agregação de Consultas** | **Scatter/Gather** | Responde a consultas complexas que exigem a coleta de dados de múltiplas fontes distribuídas. O componente implementado (o nó Raíz) distribui a requisição para diferentes réplicas ou *shards*, coleta as respostas parciais e combina-as em uma resposta unificada para o cliente. | INE5645\_aula15 |
| **Repositório de Dados Históricos** | **Particionamento (Sharding)** | Endereça o problema do banco de dados estar excessivamente grande e lento. O *Sharding* particiona e distribui os dados do sistema, reduzindo o custo de armazenamento e aumentando o desempenho ao dividir a carga de requisições. | INE5645\_aula15 |

### 4. Requisitos de Implementação e Entrega

#### 4.1. Requisitos de Código e Ambiente

*   **Implementação Mínima:** A solução deve focar em, pelo menos, três dos módulos/componentes listados na tabela acima.
*   **Natureza Distribuída:** A solução deve ser **genuinamente distribuída**, operando com múltiplos processos ou máquinas virtuais (VMs).
*   **Linguagem:** A linguagem de programação é de escolha livre.
*   **Grupos:** O trabalho deve ser realizado em grupos de até 3 participantes.

#### 4.2. Documentação e Justificativa

A entrega do projeto deve incluir o código-fonte e um Relatório Técnico (ou uma seção detalhada no README) que apresente:

1.  **Estratégia de Implementação:** Uma descrição detalhada da arquitetura distribuída adotada e como os componentes se comunicam.
2.  **Justificativa dos Padrões:** Para cada padrão implementado (*Sharding, Pub/Sub, Circuit Breaker, Scatter/Gather*), o grupo deve fornecer uma justificativa formal:
    *   **Problema Resolvido:** Explicação do problema específico que o padrão aborda (e.g., a proteção contra falhas em cascata no Circuit Breaker).
    *   **Adoção:** Descrição de como o padrão foi implementado na prática (e.g., como foram definidas as regras de distribuição para o *Sharding*, ou como o *Broker* realiza a gestão de mensagens no *Pub/Sub*).
3.  **README Detalhado:** Deve conter instruções claras para a compilação, implantação (incluindo a configuração das réplicas, *shards* e *broker*) e execução da aplicação.

#### 4.3. Avaliação

A avaliação será baseada na entrega do código-fonte, no Relatório Técnico e em uma Defesa em Aula, onde as decisões de projeto e a funcionalidade dos padrões aplicados deverão ser explicadas.
