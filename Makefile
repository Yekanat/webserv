# ********** MAKEFILE **********
# Compila l'eseguibile 'webserv'

NAME = webserv
CXX = g++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iinclude

SRC = src/main.cpp src/ConfigParser.cpp src/ServerInstance.cpp src/Server.cpp \
      src/HttpRequest.cpp src/HttpResponse.cpp src/utils.cpp
OBJ = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
