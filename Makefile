CXX = clang++
CXXFLAGS = -std=c++11 -Wall -Wextra -Werror -pedantic
NAME = Matt_daemon

# Source files
SRCS = main.cpp \
       MattDaemon.cpp \
       Server.cpp \
       Tintin_reporter.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Test files
TEST_DIR = test
TEST_SRCS = $(TEST_DIR)/test_server.cpp Server.cpp Tintin_reporter.cpp
TEST_NAME = test_server

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TEST_NAME)

$(TEST_NAME): $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) $(TEST_SRCS) -o $(TEST_NAME)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME) $(TEST_NAME)

re: fclean all

.PHONY: all clean fclean re test