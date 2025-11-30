#include "circuit_breaker.h"
#include <stdio.h>

void cb_init(CircuitBreaker *cb, int threshold, int timeout) {
    cb->state = CB_CLOSED;
    cb->failure_count = 0;
    cb->failure_threshold = threshold;
    cb->reset_timeout = timeout;
    cb->last_failure_time = 0;
}

int cb_allow_request(CircuitBreaker *cb) {
    if (cb->state == CB_CLOSED) {
        return 1; 
    }

    if (cb->state == CB_OPEN) {
        time_t now = time(NULL);
        // Verifica se o timeout já passou
        if ((now - cb->last_failure_time) > cb->reset_timeout) {
            cb->state = CB_HALF_OPEN;
            printf("[CIRCUIT BREAKER] Timeout expirou! Mudando para HALF-OPEN (Teste).\n");
            return 1; 
        }
        return 0;
    }

    if (cb->state == CB_HALF_OPEN) {
        return 1;
    }

    return 0;
}

void cb_record_success(CircuitBreaker *cb) {
    if (cb->state != CB_CLOSED) {
        printf("[CIRCUIT BREAKER] Sucesso detectado! Circuito FECHADO (Normalizado).\n");
        cb->state = CB_CLOSED;
        cb->failure_count = 0;
    }
    cb->failure_count = 0; 
}

void cb_record_failure(CircuitBreaker *cb) {
    cb->failure_count++;
    cb->last_failure_time = time(NULL);

    printf("[CIRCUIT BREAKER] Falha registrada (%d/%d).\n", cb->failure_count, cb->failure_threshold);

    if (cb->state == CB_HALF_OPEN) {
        printf("[CIRCUIT BREAKER] Falha durante teste (Half-Open). Voltando para OPEN.\n");
        cb->state = CB_OPEN;
    } 
    else if (cb->failure_count >= cb->failure_threshold) {
        if (cb->state == CB_CLOSED) {
            printf("[CIRCUIT BREAKER] Limite atingido! Circuito ABERTO. Bloqueando requisições por %ds.\n", cb->reset_timeout);
        }
        cb->state = CB_OPEN;
    }
}

const char* cb_state_to_string(BreakerState state) {
    switch (state) {
        case CB_CLOSED: return "FECHADO (Normal)";
        case CB_OPEN: return "ABERTO (Falha)";
        case CB_HALF_OPEN: return "MEIO-ABERTO (Testando)";
        default: return "DESCONHECIDO";
    }
}