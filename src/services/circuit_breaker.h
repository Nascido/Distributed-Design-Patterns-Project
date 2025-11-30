// circuit_breaker.h
#ifndef CIRCUIT_BREAKER_H
#define CIRCUIT_BREAKER_H

#include <time.h>

typedef enum {
    CB_CLOSED,      
    CB_OPEN,        
    CB_HALF_OPEN    
} BreakerState;

typedef struct {
    BreakerState state;
    int failure_count;          
    int failure_threshold;      
    time_t last_failure_time;   
    int reset_timeout;          
} CircuitBreaker;


void cb_init(CircuitBreaker *cb, int threshold, int timeout);

int cb_allow_request(CircuitBreaker *cb);

void cb_record_success(CircuitBreaker *cb);

void cb_record_failure(CircuitBreaker *cb);

const char* cb_state_to_string(BreakerState state);

#endif