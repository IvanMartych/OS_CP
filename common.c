#include "common.h"

void init_message(Message *msg) {
    memset(msg, 0, sizeof(Message));
}

int send_message(void *socket, Message *msg) {
    int rc = zmq_send(socket, msg, sizeof(Message), 0);
    return rc;
}

int recv_message(void *socket, Message *msg) {
    int rc = zmq_recv(socket, msg, sizeof(Message), 0);
    return rc;
}

void generate_secret(int *secret) {
    int used[10] = {0};
    srand(time(NULL) ^ (unsigned int)(uintptr_t)secret);
    
    for (int i = 0; i < SECRET_LENGTH; i++) {
        int digit;
        do {
            digit = rand() % 10;
        } while (used[digit]);
        
        used[digit] = 1;
        secret[i] = digit;
    }
}

void calculate_bulls_cows(int *secret, int *guess, int *bulls, int *cows) {
    *bulls = 0;
    *cows = 0;
    
    for (int i = 0; i < SECRET_LENGTH; i++) {
        if (secret[i] == guess[i]) {
            (*bulls)++;
        } else {
            for (int j = 0; j < SECRET_LENGTH; j++) {
                if (i != j && secret[i] == guess[j]) {
                    (*cows)++;
                    break;
                }
            }
        }
    }
}

int is_valid_number(int *number) {
    int used[10] = {0};
    
    for (int i = 0; i < SECRET_LENGTH; i++) {
        if (number[i] < 0 || number[i] > 9) {
            return 0;
        }
        if (used[number[i]]) {
            return 0; // Дубликат
        }
        used[number[i]] = 1;
    }
    
    return 1;
}
