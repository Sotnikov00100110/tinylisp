all: 
    gcc src/main.c -o tinylisp
    clang src/main.c -o tinylisp 
clean: 
    rm -f tinylisp 
