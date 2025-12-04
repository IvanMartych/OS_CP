#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <zmq.h>

#define MAX_GAME_NAME 64
#define MAX_PLAYER_NAME 32
#define MAX_PLAYERS 10
#define SECRET_LENGTH 4
#define MAX_ATTEMPTS 100

// Типы сообщений
typedef enum {
    MSG_CREATE_GAME,
    MSG_JOIN_GAME,
    MSG_FIND_GAME,
    MSG_MAKE_GUESS,
    MSG_LEAVE_GAME,
    MSG_LIST_GAMES,
    
    // Ответы сервера
    MSG_GAME_CREATED,
    MSG_JOINED_GAME,
    MSG_GAME_FOUND,
    MSG_GUESS_RESULT,
    MSG_GAME_WON,
    MSG_GAME_STATE,
    MSG_ERROR,
    MSG_GAME_LIST
} MessageType;

// Структура для результата попытки
typedef struct {
    int bulls;
    int cows;
    int attempt_number;
    char player_name[MAX_PLAYER_NAME];
} GuessResult;

// Структура сообщения
typedef struct {
    MessageType type;
    char game_name[MAX_GAME_NAME];
    char player_name[MAX_PLAYER_NAME];
    int max_players;
    int guess[SECRET_LENGTH];
    GuessResult result;
    char error_msg[256];
    int game_count;
    int player_count;
    int is_winner;
} Message;

// Функции для работы с сообщениями
void init_message(Message *msg);
int send_message(void *socket, Message *msg);
int recv_message(void *socket, Message *msg);

// Утилиты для игры
void generate_secret(int *secret);
void calculate_bulls_cows(int *secret, int *guess, int *bulls, int *cows);
int is_valid_number(int *number);

#endif // COMMON_H
