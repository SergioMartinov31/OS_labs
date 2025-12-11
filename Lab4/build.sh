#!/bin/bash

echo "=== Building libraries ==="

# Создаем директории
mkdir -p lib1 lib2

# Компилируем первую библиотеку (БЕЗ .o файлов!)
echo "Building lib1..."
gcc -fPIC -shared -Wall lib1/prime.c lib1/translate.c -o lib1/lib1.so

# Компилируем вторую библиотеку (БЕЗ .o файлов!)
echo "Building lib2..."
gcc -fPIC -shared -Wall lib2/prime.c lib2/translate.c -o lib2/lib2.so

echo "=== Building programs ==="

# Программа 1 (статическая линковка с lib1)
echo "Building prog1..."
gcc -Wall -I. prog1.c -L./lib1 -l1 -o prog1

# Программа 2 (динамическая загрузка)
echo "Building prog2..."
gcc -Wall -I. prog2.c -ldl -o prog2

echo "=== Done! ==="
echo "To run prog1: LD_LIBRARY_PATH=./lib1 ./prog1"
echo "To run prog2: ./prog2"



# ./build.sh

# prog1
# 1 10 30    
# 2 42      

#prog2
# 1 10 30
# 2 42
# 0
# 1 10 30
# 2 42