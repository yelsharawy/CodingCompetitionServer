all: game

game: game.o
	gcc -o game game.o

game.o: game.c
	gcc -c game.c

run:
	./game
# 	./play_game.sh

clean:
	-rm a b in1 in2 out1 out2 2> /dev/null