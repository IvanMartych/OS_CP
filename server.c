#include "common.h"
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#define SERVER_ENDPOINT "tcp://*:5555"
#define MAX_THREADS 100

// Структура игрока
typedef struct {
    char name[MAX_PLAYER_NAME];
    int is_active;
    int attempts;
} Player;

// Структура игры
typedef struct {
    char name[MAX_GAME_NAME];
    int secret[SECRET_LENGTH];
    int max_players;
    int current_players;
    Player players[MAX_PLAYERS];
    int is_active;
    int is_finished;
    char winner[MAX_PLAYER_NAME];
} Game;

// Глобальные переменные
Game games[100];
int game_count = 0;
int running = 1;
pthread_mutex_t games_mutex = PTHREAD_MUTEX_INITIALIZER;
void *global_context = NULL;

// Структура для передачи данных в поток
typedef struct {
    void *socket;
    Message request;
    char identity[256];
    int identity_size;
} ClientRequest;

void signal_handler(int signum) {
    printf("\nПолучен сигнал %d. Завершение работы сервера...\n", signum);
    running = 0;
}

Game* find_game_by_name(const char *name) {
    pthread_mutex_lock(&games_mutex);
    Game *result = NULL;
    for (int i = 0; i < game_count; i++) {
        if (strcmp(games[i].name, name) == 0 && games[i].is_active) {
            result = &games[i];
            break;
        }
    }
    pthread_mutex_unlock(&games_mutex);
    return result;
}

Game* find_available_game() {
    pthread_mutex_lock(&games_mutex);
    Game *result = NULL;
    for (int i = 0; i < game_count; i++) {
        if (games[i].is_active && 
            games[i].current_players < games[i].max_players &&
            !games[i].is_finished) {
            result = &games[i];
            break;
        }
    }
    pthread_mutex_unlock(&games_mutex);
    return result;
}

void handle_create_game(Message *request, Message *response) {
    pthread_mutex_lock(&games_mutex);
    
    if (game_count >= 100) {
        pthread_mutex_unlock(&games_mutex);
        response->type = MSG_ERROR;
        strcpy(response->error_msg, "Достигнут лимит игр на сервере");
        return;
    }
    
    // Проверка существования игры
    int game_exists = 0;
    for (int i = 0; i < game_count; i++) {
        if (strcmp(games[i].name, request->game_name) == 0 && games[i].is_active) {
            game_exists = 1;
            break;
        }
    }
    
    if (game_exists) {
        pthread_mutex_unlock(&games_mutex);
        response->type = MSG_ERROR;
        strcpy(response->error_msg, "Игра с таким именем уже существует");
        return;
    }
    
    if (request->max_players < 1 || request->max_players > MAX_PLAYERS) {
        pthread_mutex_unlock(&games_mutex);
        response->type = MSG_ERROR;
        sprintf(response->error_msg, "Количество игроков должно быть от 1 до %d", MAX_PLAYERS);
        return;
    }
    
    Game *game = &games[game_count++];
    strcpy(game->name, request->game_name);
    game->max_players = request->max_players;
    game->current_players = 1;
    game->is_active = 1;
    game->is_finished = 0;
    game->winner[0] = '\0';
    
    // Добавляем создателя как первого игрока
    strcpy(game->players[0].name, request->player_name);
    game->players[0].is_active = 1;
    game->players[0].attempts = 0;
    
    // Генерируем секретное число
    generate_secret(game->secret);
    
    printf("Создана игра '%s' с секретным числом: %d%d%d%d\n", 
           game->name, game->secret[0], game->secret[1], 
           game->secret[2], game->secret[3]);
    
    response->type = MSG_GAME_CREATED;
    strcpy(response->game_name, game->name);
    response->max_players = game->max_players;
    response->player_count = game->current_players;
    
    pthread_mutex_unlock(&games_mutex);
}

void handle_join_game(Message *request, Message *response) {
    pthread_mutex_lock(&games_mutex);
    
    Game *game = NULL;
    for (int i = 0; i < game_count; i++) {
        if (strcmp(games[i].name, request->game_name) == 0 && games[i].is_active) {
            game = &games[i];
            break;
        }
    }
    
    if (game == NULL) {
        pthread_mutex_unlock(&games_mutex);
        response->type = MSG_ERROR;
        strcpy(response->error_msg, "Игра не найдена");
        return;
    }
    
    if (game->is_finished) {
        pthread_mutex_unlock(&games_mutex);
        response->type = MSG_ERROR;
        strcpy(response->error_msg, "Игра уже завершена");
        return;
    }
    
    if (game->current_players >= game->max_players) {
        pthread_mutex_unlock(&games_mutex);
        response->type = MSG_ERROR;
        strcpy(response->error_msg, "Игра заполнена");
        return;
    }
    
    // Проверяем, не присоединился ли игрок уже
    for (int i = 0; i < game->current_players; i++) {
        if (strcmp(game->players[i].name, request->player_name) == 0) {
            pthread_mutex_unlock(&games_mutex);
            response->type = MSG_ERROR;
            strcpy(response->error_msg, "Вы уже в этой игре");
            return;
        }
    }
    
    // Добавляем игрока
    int idx = game->current_players++;
    strcpy(game->players[idx].name, request->player_name);
    game->players[idx].is_active = 1;
    game->players[idx].attempts = 0;
    
    printf("Игрок '%s' присоединился к игре '%s' (%d/%d)\n", 
           request->player_name, game->name, 
           game->current_players, game->max_players);
    
    response->type = MSG_JOINED_GAME;
    strcpy(response->game_name, game->name);
    response->max_players = game->max_players;
    response->player_count = game->current_players;
    
    pthread_mutex_unlock(&games_mutex);
}

void handle_find_game(Message *request, Message *response) {
    pthread_mutex_lock(&games_mutex);
    
    Game *game = NULL;
    for (int i = 0; i < game_count; i++) {
        if (games[i].is_active && 
            games[i].current_players < games[i].max_players &&
            !games[i].is_finished) {
            game = &games[i];
            break;
        }
    }
    
    if (game == NULL) {
        pthread_mutex_unlock(&games_mutex);
        response->type = MSG_ERROR;
        strcpy(response->error_msg, "Нет доступных игр. Создайте новую игру.");
        return;
    }
    
    // Проверяем, не присоединился ли игрок уже
    for (int i = 0; i < game->current_players; i++) {
        if (strcmp(game->players[i].name, request->player_name) == 0) {
            pthread_mutex_unlock(&games_mutex);
            response->type = MSG_ERROR;
            strcpy(response->error_msg, "Вы уже в этой игре");
            return;
        }
    }
    
    // Добавляем игрока
    int idx = game->current_players++;
    strcpy(game->players[idx].name, request->player_name);
    game->players[idx].is_active = 1;
    game->players[idx].attempts = 0;
    
    printf("Игрок '%s' автоматически присоединился к игре '%s' (%d/%d)\n", 
           request->player_name, game->name, 
           game->current_players, game->max_players);
    
    response->type = MSG_GAME_FOUND;
    strcpy(response->game_name, game->name);
    response->max_players = game->max_players;
    response->player_count = game->current_players;
    
    pthread_mutex_unlock(&games_mutex);
}

void handle_make_guess(Message *request, Message *response) {
    pthread_mutex_lock(&games_mutex);
    
    Game *game = NULL;
    for (int i = 0; i < game_count; i++) {
        if (strcmp(games[i].name, request->game_name) == 0 && games[i].is_active) {
            game = &games[i];
            break;
        }
    }
    
    if (game == NULL) {
        pthread_mutex_unlock(&games_mutex);
        response->type = MSG_ERROR;
        strcpy(response->error_msg, "Игра не найдена");
        return;
    }
    
    if (game->is_finished) {
        pthread_mutex_unlock(&games_mutex);
        response->type = MSG_ERROR;
        sprintf(response->error_msg, "Игра завершена. Победитель: %s", game->winner);
        return;
    }
    
    // Находим игрока
    Player *player = NULL;
    for (int i = 0; i < game->current_players; i++) {
        if (strcmp(game->players[i].name, request->player_name) == 0) {
            player = &game->players[i];
            break;
        }
    }
    
    if (player == NULL) {
        pthread_mutex_unlock(&games_mutex);
        response->type = MSG_ERROR;
        strcpy(response->error_msg, "Вы не участвуете в этой игре");
        return;
    }
    
    // Валидация числа
    if (!is_valid_number(request->guess)) {
        pthread_mutex_unlock(&games_mutex);
        response->type = MSG_ERROR;
        strcpy(response->error_msg, "Неверный формат числа (все цифры должны быть уникальными)");
        return;
    }
    
    player->attempts++;
    
    int bulls, cows;
    calculate_bulls_cows(game->secret, request->guess, &bulls, &cows);
    
    printf("Игрок '%s' в игре '%s': попытка %d - %d%d%d%d -> %dБ %dК\n",
           player->name, game->name, player->attempts,
           request->guess[0], request->guess[1], 
           request->guess[2], request->guess[3],
           bulls, cows);
    
    response->result.bulls = bulls;
    response->result.cows = cows;
    response->result.attempt_number = player->attempts;
    strcpy(response->result.player_name, player->name);
    
    if (bulls == SECRET_LENGTH) {
        game->is_finished = 1;
        strcpy(game->winner, player->name);
        response->type = MSG_GAME_WON;
        response->is_winner = 1;
        printf("*** Игрок '%s' выиграл игру '%s' за %d попыток! ***\n", 
               player->name, game->name, player->attempts);
    } else {
        response->type = MSG_GUESS_RESULT;
        response->is_winner = 0;
    }
    
    strcpy(response->game_name, game->name);
    
    pthread_mutex_unlock(&games_mutex);
}

void handle_list_games(Message *request, Message *response) {
    pthread_mutex_lock(&games_mutex);
    
    response->type = MSG_GAME_LIST;
    response->game_count = 0;
    
    printf("Запрос списка игр. Активных игр: ");
    for (int i = 0; i < game_count; i++) {
        if (games[i].is_active && !games[i].is_finished) {
            response->game_count++;
        }
    }
    printf("%d\n", response->game_count);
    
    pthread_mutex_unlock(&games_mutex);
}

void process_message(Message *request, Message *response) {
    init_message(response);
    
    switch (request->type) {
        case MSG_CREATE_GAME:
            handle_create_game(request, response);
            break;
        case MSG_JOIN_GAME:
            handle_join_game(request, response);
            break;
        case MSG_FIND_GAME:
            handle_find_game(request, response);
            break;
        case MSG_MAKE_GUESS:
            handle_make_guess(request, response);
            break;
        case MSG_LIST_GAMES:
            handle_list_games(request, response);
            break;
        default:
            response->type = MSG_ERROR;
            strcpy(response->error_msg, "Неизвестный тип сообщения");
            break;
    }
}

// Обработчик клиента в отдельном потоке
void* handle_client(void* arg) {
    ClientRequest *client_req = (ClientRequest*)arg;
    Message response;
    
    // Обрабатываем запрос
    process_message(&client_req->request, &response);
    
    // Отправляем ответ обратно через ROUTER сокет
    zmq_send(client_req->socket, client_req->identity, client_req->identity_size, ZMQ_SNDMORE);
    zmq_send(client_req->socket, "", 0, ZMQ_SNDMORE);
    
    // Сериализуем и отправляем ответ
    size_t msg_size = sizeof(Message);
    zmq_send(client_req->socket, &response, msg_size, 0);
    
    free(client_req);
    return NULL;
}

int main() {
    printf("========================================\n");
    printf("Сервер игры 'Быки и Коровы' (многопоточный)\n");
    printf("========================================\n\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    global_context = zmq_ctx_new();
    void *socket = zmq_socket(global_context, ZMQ_ROUTER);
    
    int rc = zmq_bind(socket, SERVER_ENDPOINT);
    if (rc != 0) {
        printf("Ошибка привязки сокета: %s\n", zmq_strerror(errno));
        return 1;
    }
    
    printf("Сервер запущен на %s\n", SERVER_ENDPOINT);
    printf("Режим: многопоточный (до %d потоков)\n", MAX_THREADS);
    printf("Ожидание подключений...\n\n");
    
    // Установка таймаута для recv
    int timeout = 1000; // 1 секунда
    zmq_setsockopt(socket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    
    while (running) {
        ClientRequest *client_req = malloc(sizeof(ClientRequest));
        if (!client_req) {
            printf("Ошибка выделения памяти\n");
            continue;
        }
        
        // Получаем identity клиента
        client_req->identity_size = zmq_recv(socket, client_req->identity, 256, 0);
        if (client_req->identity_size == -1) {
            if (zmq_errno() == EAGAIN) {
                free(client_req);
                continue;
            }
            printf("Ошибка получения identity: %s\n", zmq_strerror(errno));
            free(client_req);
            break;
        }
        
        // Пропускаем разделитель
        char delimiter[10];
        rc = zmq_recv(socket, delimiter, 10, 0);
        if (rc == -1) {
            printf("Ошибка получения разделителя: %s\n", zmq_strerror(errno));
            free(client_req);
            continue;
        }
        
        // Получаем сообщение
        size_t msg_size = sizeof(Message);
        rc = zmq_recv(socket, &client_req->request, msg_size, 0);
        if (rc == -1) {
            printf("Ошибка получения сообщения: %s\n", zmq_strerror(errno));
            free(client_req);
            continue;
        }
        
        client_req->socket = socket;
        
        // Создаем новый поток для обработки запроса
        pthread_t thread;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        
        if (pthread_create(&thread, &attr, handle_client, client_req) != 0) {
            printf("Ошибка создания потока\n");
            free(client_req);
        }
        
        pthread_attr_destroy(&attr);
    }
    
    printf("\nЗакрытие сервера...\n");
    zmq_close(socket);
    zmq_ctx_destroy(global_context);
    pthread_mutex_destroy(&games_mutex);
    
    printf("Сервер остановлен.\n");
    return 0;
}
