### AUTOR: MARTIN HLINSKY
### PROJEKT: IOS 2 - Faneuil Hall Problem

CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic
LFLAGS=-pthread -lrt
NAME=proj2

$(NAME): $(NAME).c
	$(CC) $(CFLAGS) $(NAME).c -lm -o $(NAME) $(LFLAGS)
