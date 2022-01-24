all: game_runner reranker submissions containers

game_runner: game_runner.o submissions.o containers.o
	gcc -o game_runner game_runner.o submissions.o containers.o
game_runner.o: game_runner.c common.h containers.h submissions.h
	gcc -c game_runner.c

reranker: reranker.o
	gcc -o reranker reranker.o -lm
reranker.o: reranker.c common.h
	gcc -c reranker.c

submissions: submissions.o containers.o
	gcc -o submissions submissions.o containers.o
submissions.o: submissions.c containers.h common.h submissions.h
	gcc -c submissions.c

containers: containers.o
	gcc -o containers containers.o
containers.o: containers.c containers.h common.h
	gcc -c containers.c

run:
	./game_runner tic-tac-toe/game tic-tac-toe/random_player.py tic-tac-toe/random_player.py

clean:
	-rm -rf running_*/
	-rm -rf testcontainer_*/
	-rm ./*.o