#include "common.h"

#define SERVER_ENDPOINT "tcp://localhost:5555"

// Прототипы функций
void play_game(void *socket, const char *player_name, const char *game_name);

void print_menu() {
    printf("\n========================================\n");
    printf("      Игра 'Быки и Коровы'\n");
    printf("========================================\n");
    printf("1. Создать новую игру\n");
    printf("2. Присоединиться к игре по имени\n");
    printf("3. Найти доступную игру (автопоиск)\n");
    printf("4. Список активных игр\n");
    printf("5. Выход\n");
    printf("========================================\n");
    printf("Выберите действие: ");
}

void print_game_rules() {
    printf("\n========================================\n");
    printf("         ПРАВИЛА ИГРЫ\n");
    printf("========================================\n");
    printf("Цель: угадать 4-значное число с\n");
    printf("      уникальными цифрами (0-9)\n\n");
    printf("БЫК  - правильная цифра на\n");
    printf("       правильной позиции\n");
    printf("КОРОВА - правильная цифра на\n");
    printf("         неправильной позиции\n\n");
    printf("Пример:\n");
    printf("  Секрет: 1234\n");
    printf("  Попытка: 1567 -> 1 бык, 0 коров\n");
    printf("  Попытка: 2341 -> 0 быков, 4 коровы\n");
    printf("  Попытка: 1234 -> 4 быка (ПОБЕДА!)\n");
    printf("========================================\n\n");
}

int read_number(int *number) {
    char input[100];
    printf("Введите 4-значное число: ");
    
    if (fgets(input, sizeof(input), stdin) == NULL) {
        return 0;
    }
    
    // Удаляем перевод строки
    input[strcspn(input, "\n")] = 0;
    
    if (strlen(input) != SECRET_LENGTH) {
        printf("Ошибка: нужно ввести ровно %d цифры\n", SECRET_LENGTH);
        return 0;
    }
    
    for (int i = 0; i < SECRET_LENGTH; i++) {
        if (input[i] < '0' || input[i] > '9') {
            printf("Ошибка: используйте только цифры 0-9\n");
            return 0;
        }
        number[i] = input[i] - '0';
    }
    
    if (!is_valid_number(number)) {
        printf("Ошибка: все цифры должны быть уникальными\n");
        return 0;
    }
    
    return 1;
}

void create_game(void *socket, const char *player_name) {
    Message request, response;
    init_message(&request);
    
    request.type = MSG_CREATE_GAME;
    strcpy(request.player_name, player_name);
    
    printf("\nВведите название игры: ");
    if (fgets(request.game_name, sizeof(request.game_name), stdin) == NULL) {
        printf("Ошибка ввода\n");
        return;
    }
    request.game_name[strcspn(request.game_name, "\n")] = 0;
    
    printf("Введите максимальное количество игроков (1-%d): ", MAX_PLAYERS);
    if (scanf("%d", &request.max_players) != 1) {
        printf("Ошибка ввода\n");
        while (getchar() != '\n');
        return;
    }
    while (getchar() != '\n'); // Очистка буфера
    
    if (request.max_players < 1 || request.max_players > MAX_PLAYERS) {
        printf("Ошибка: количество игроков должно быть от 1 до %d\n", MAX_PLAYERS);
        return;
    }
    
    printf("Отправка запроса на сервер...\n");
    send_message(socket, &request);
    
    printf("Ожидание ответа от сервера...\n");
    recv_message(socket, &response);
    
    if (response.type == MSG_ERROR) {
        printf("Ошибка: %s\n", response.error_msg);
        return;
    }
    
    printf("\n✓ Игра '%s' создана успешно!\n", response.game_name);
    printf("Игроков: %d/%d\n", response.player_count, response.max_players);
    
    // Начинаем игру
    play_game(socket, player_name, response.game_name);
}

void join_game(void *socket, const char *player_name) {
    Message request, response;
    init_message(&request);
    
    request.type = MSG_JOIN_GAME;
    strcpy(request.player_name, player_name);
    
    printf("\nВведите название игры: ");
    if (fgets(request.game_name, sizeof(request.game_name), stdin) == NULL) {
        printf("Ошибка ввода\n");
        return;
    }
    request.game_name[strcspn(request.game_name, "\n")] = 0;
    
    send_message(socket, &request);
    recv_message(socket, &response);
    
    if (response.type == MSG_ERROR) {
        printf("Ошибка: %s\n", response.error_msg);
        return;
    }
    
    printf("\n✓ Вы присоединились к игре '%s'!\n", response.game_name);
    printf("Игроков: %d/%d\n", response.player_count, response.max_players);
    
    // Начинаем игру
    play_game(socket, player_name, response.game_name);
}

void find_game(void *socket, const char *player_name) {
    Message request, response;
    init_message(&request);
    
    request.type = MSG_FIND_GAME;
    strcpy(request.player_name, player_name);
    
    printf("\nПоиск доступной игры...\n");
    
    send_message(socket, &request);
    recv_message(socket, &response);
    
    if (response.type == MSG_ERROR) {
        printf("Ошибка: %s\n", response.error_msg);
        return;
    }
    
    printf("\n✓ Найдена игра '%s'!\n", response.game_name);
    printf("Игроков: %d/%d\n", response.player_count, response.max_players);
    
    // Начинаем игру
    play_game(socket, player_name, response.game_name);
}

void list_games(void *socket) {
    Message request, response;
    init_message(&request);
    
    request.type = MSG_LIST_GAMES;
    
    send_message(socket, &request);
    recv_message(socket, &response);
    
    printf("\nАктивных игр на сервере: %d\n", response.game_count);
}

void play_game(void *socket, const char *player_name, const char *game_name) {
    print_game_rules();
    
    printf("Начинаем игру! Попытайтесь угадать число.\n");
    printf("Введите 'q' чтобы выйти из игры\n\n");
    
    int attempt = 0;
    char input[100];
    
    while (1) {
        printf("\n--- Попытка %d ---\n", attempt + 1);
        
        Message request, response;
        init_message(&request);
        
        request.type = MSG_MAKE_GUESS;
        strcpy(request.player_name, player_name);
        strcpy(request.game_name, game_name);
        
        if (!read_number(request.guess)) {
            printf("Попробуйте снова\n");
            continue;
        }
        
        send_message(socket, &request);
        recv_message(socket, &response);
        
        if (response.type == MSG_ERROR) {
            printf("Ошибка: %s\n", response.error_msg);
            break;
        }
        
        attempt++;
        
        printf("\nРезультат: %d БЫКОВ, %d КОРОВ\n", 
               response.result.bulls, response.result.cows);
        
        if (response.type == MSG_GAME_WON) {
            printf("\n");
            printf("****************************************\n");
            printf("***      ПОЗДРАВЛЯЕМ! ВЫ ВЫИГРАЛИ!   ***\n");
            printf("***   Угадано за %d попыток!          ***\n", attempt);
            printf("****************************************\n");
            break;
        }
        
        printf("Попыток: %d\n", attempt);
    }
    
    printf("\nИгра окончена. Нажмите Enter...");
    getchar();
}

int main() {
    char player_name[MAX_PLAYER_NAME];
    
    printf("========================================\n");
    printf("  Клиент игры 'Быки и Коровы'\n");
    printf("========================================\n");
    
    printf("\nВведите ваше имя: ");
    if (fgets(player_name, sizeof(player_name), stdin) == NULL) {
        printf("Ошибка ввода\n");
        return 1;
    }
    player_name[strcspn(player_name, "\n")] = 0;
    
    if (strlen(player_name) == 0) {
        strcpy(player_name, "Игрок");
    }
    
    printf("Добро пожаловать, %s!\n", player_name);
    printf("Подключение к серверу...\n");
    
    void *context = zmq_ctx_new();
    void *socket = zmq_socket(context, ZMQ_REQ);
    
    int rc = zmq_connect(socket, SERVER_ENDPOINT);
    if (rc != 0) {
        printf("Ошибка подключения к серверу: %s\n", zmq_strerror(errno));
        return 1;
    }
    
    printf("✓ Подключено к серверу\n");
    
    int choice;
    while (1) {
        print_menu();
        
        if (scanf("%d", &choice) != 1) {
            printf("Ошибка ввода\n");
            while (getchar() != '\n');
            continue;
        }
        while (getchar() != '\n'); // Очистка буфера
        
        switch (choice) {
            case 1:
                create_game(socket, player_name);
                break;
            case 2:
                join_game(socket, player_name);
                break;
            case 3:
                find_game(socket, player_name);
                break;
            case 4:
                list_games(socket);
                break;
            case 5:
                printf("\nДо свидания!\n");
                zmq_close(socket);
                zmq_ctx_destroy(context);
                return 0;
            default:
                printf("Неверный выбор\n");
        }
    }
    
    zmq_close(socket);
    zmq_ctx_destroy(context);
    return 0;
}