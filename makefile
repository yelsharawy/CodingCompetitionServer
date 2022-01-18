all: game_runner reranker

game_runner: game_runner.o
	gcc -o game_runner game_runner.o
game_runner.o: game_runner.c
	gcc -c game_runner.c

reranker: reranker.o
	gcc -o reranker reranker.o -lm
reranker.o: reranker.c constants.h
	gcc -c reranker.c

run:
	./game_runner tic-tac-toe/game tic-tac-toe/random_player.py tic-tac-toe/random_player.py

clean:
	-rm -rf running_*/
	-rm ./*.o