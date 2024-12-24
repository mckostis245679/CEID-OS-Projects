all:
	gcc launch.c passenger.c -pthread -o output/launch -Wall -Werror -Wextra && ./output/launch

debug:
	gcc launch.c passenger.c -pthread -g -o output/launch -Wall -Werror -Wextra && ./output/launch


