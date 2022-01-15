game_runner: game_runner.o
	gcc -o game_runner game_runner.o

game_runner.o: game_runner.c
	gcc -c game_runner.c

run:
	./game_runner tic-tac-toe/game tic-tac-toe/random_player.py tic-tac-toe/random_player.py