#!/bin/bash

# Скрипт автоматической сборки проекта с CMake

echo "=========================================="
echo "   Сборка проекта 'Быки и Коровы'"
echo "=========================================="
echo ""

# Проверка наличия cmake
if ! command -v cmake &> /dev/null; then
    echo "❌ CMake не найден!"
    echo "Установите: sudo apt-get install cmake"
    exit 1
fi

# Проверка наличия ZeroMQ (проверяем наличие заголовочного файла)
if [ ! -f "/usr/include/zmq.h" ] && [ ! -f "/usr/local/include/zmq.h" ]; then
    echo "❌ ZeroMQ не найден!"
    echo "Установите: sudo apt-get install libzmq3-dev"
    exit 1
fi

echo "✅ Все зависимости найдены"
echo ""

# Создание папки сборки
if [ -d "build" ]; then
    echo "Папка build/ уже существует"
    read -p "Удалить и пересобрать? (y/n): " choice
    if [ "$choice" = "y" ] || [ "$choice" = "Y" ]; then
        echo "Удаление старой сборки..."
        rm -rf build
    else
        echo "Использование существующей сборки..."
    fi
fi

if [ ! -d "build" ]; then
    echo "Создание папки сборки..."
    mkdir build
fi

cd build

# Настройка проекта
echo ""
echo "Настройка проекта с CMake..."
cmake .. || {
    echo "❌ Ошибка настройки CMake!"
    exit 1
}

# Сборка
echo ""
echo "Сборка проекта..."
make -j$(nproc) || {
    echo "❌ Ошибка сборки!"
    exit 1
}

echo ""
echo "=========================================="
echo "✅ Сборка успешно завершена!"
echo "=========================================="
echo ""
echo "Исполняемые файлы:"
echo "  - ./build/server"
echo "  - ./build/client"
echo ""
echo "Для запуска:"
echo "  Терминал 1: cd build && ./server"
echo "  Терминал 2: cd build && ./client"
echo ""
