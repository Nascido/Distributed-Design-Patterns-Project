// circuit_breaker.h
#ifndef CIRCUIT_BREAKER_H
#define CIRCUIT_BREAKER_H

#include <time.h>

// Estados do Disjuntor
typedef enum {
    CB_CLOSED,      // Tudo normal, requisições passam (0)
    CB_OPEN,        // Falha detectada, requisições bloqueadas (1)
    CB_HALF_OPEN    // Tempo de espera acabou, testando uma requisição (2)
} BreakerState;

// Estrutura de Controle do Disjuntor
typedef struct {
    BreakerState state;
    int failure_count;          // Quantas falhas seguidas ocorreram
    int failure_threshold;      // Limite de falhas antes de abrir (ex: 3)
    time_t last_failure_time;   // Timestamp da última falha
    int reset_timeout;          // Segundos para esperar antes de tentar de novo (Half-Open)
} CircuitBreaker;

// --- Funções Públicas ---

// Inicializa a estrutura
void cb_init(CircuitBreaker *cb, int threshold, int timeout);

// Pergunta: "Posso fazer uma requisição?" (1 = Sim, 0 = Não)
int cb_allow_request(CircuitBreaker *cb);

// Chamado quando a requisição externa funciona
void cb_record_success(CircuitBreaker *cb);

// Chamado quando a requisição externa falha
void cb_record_failure(CircuitBreaker *cb);

// Auxiliar para logs (converte estado numérico para string)
const char* cb_state_to_string(BreakerState state);

#endif