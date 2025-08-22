NAME = webserv
CC = c++
CFLAGS= -Wall -Wextra -Werror -std=c++98

SRC = parsing.cpp chekers.cpp main.cpp 

all:
	$(CC) $(CFLAGS) $(SRC) -o $(NAME)

fclean:
	rm -rf $(NAME)

re:
	fclean all