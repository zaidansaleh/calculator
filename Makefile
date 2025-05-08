calculator: calculator.c
	gcc -std=c11 -Wall -Wextra -g -o calculator calculator.c

.PHONY: clean
clean:
	rm calculator

