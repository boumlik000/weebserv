NAME = webserv
CC = c++
CFLAGS= -Wall -Wextra -Werror -std=c++98

SRC = wb0.cpp runserver.cpp main.cpp #conf.cpp

all:
	$(CC) $(CFLAGS) $(SRC) -o $(NAME)

fclean:
	rm -rf $(NAME)

re:
	fclean all