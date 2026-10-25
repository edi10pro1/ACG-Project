#!/bin/bash

# Setează directorul curent ca fiind folderul root al proiectului
cd "$(dirname "$0")"

# Detectare cale Homebrew (Apple Silicon vs Intel)
if [ -d "/opt/homebrew" ]; then
    BREW_PREFIX="/opt/homebrew"
else
    BREW_PREFIX="/usr/local"
fi

echo "Se compilează proiectul OpenGL pe macOS..."

# Compilare cu C++17 și legarea bibliotecilor native de Mac
clang++ -std=c++17 main.cpp shader.cpp -o opengl_app \
    -I"$BREW_PREFIX/include" \
    -L"$BREW_PREFIX/lib" \
    -lGLEW \
    -lglfw \
    -framework OpenGL

# Verificare compilare și lansare
if [ $? -eq 0 ]; then
    echo "Compilare reușită! Se pornește aplicația..."
    ./opengl_app
else
    echo "Eroare la compilare! Verifică mesajele de mai sus."
fi