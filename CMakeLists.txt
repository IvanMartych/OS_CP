cmake_minimum_required(VERSION 3.10)
project(BullsAndCows C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Общие исходные файлы
set(COMMON_SOURCES common.c common.h)

# Сервер
add_executable(server server.c ${COMMON_SOURCES})
target_link_libraries(server zmq pthread)

# Клиент
add_executable(client client.c ${COMMON_SOURCES})
target_link_libraries(client zmq pthread)

# Установка
install(TARGETS server client DESTINATION bin)
