all: ccs

# the commented out sections are remnants of when each one was its own program during development
# all: game_runner reranker submissions containers

ccs: main.o submissions.o containers.o reranker.o games.o users.o
	gcc -o ccs main.o submissions.o containers.o reranker.o games.o users.o -lm
main.o: main.c common.h submissions.h containers.h reranker.h games.h users.h
	gcc -c main.c

# game_runner: game_runner.o submissions.o containers.o
# 	gcc -o game_runner game_runner.o submissions.o containers.o
games.o: games.c common.h containers.h submissions.h users.h
	gcc -c games.c

# reranker: reranker.o
# 	gcc -o reranker reranker.o -lm
reranker.o: reranker.c common.h users.h
	gcc -c reranker.c

# submissions: submissions.o containers.o
# 	gcc -o submissions submissions.o containers.o
submissions.o: submissions.c containers.h common.h submissions.h
	gcc -c submissions.c

# containers: containers.o
# 	gcc -o containers containers.o
containers.o: containers.c containers.h common.h
	gcc -c containers.c

users.o: users.c common.h users.h
	gcc -c users.c

run:
	./game_runner tic-tac-toe/game tic-tac-toe/random_player.py tic-tac-toe/random_player.py

clean:
	-rm -rf running_*/
	-rm -rf testcontainer_*/
	-rm ./*.o