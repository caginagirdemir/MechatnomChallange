NAME = MechatnomChallenge

# control codes
RESET	=	\033[0m
GREEN	=	\033[32m
YELLOW	=	\033[33m
BLUE	=	\033[34m
RED		=	\033[31m
UP		=	\033[A
CUT		=	\033[K

all: 
	@echo "$(YELLOW)Compiling [$(NAME)]..."
	@c++ -Wall -Werror -Wextra program.c -o $(NAME)
	@printf "$(UP)$(CUT)"
	@echo "$(GREEN)Finished [$(NAME)]$(RESET)"

clean:
	rm -rf weapon
re: clean all
.PHONY: all, clean, re