from random import *

played = set()
me = input()

if me == 'O':
    played.add(eval(input()))

while True:
    willPlay = (randrange(3), randrange(3))
    while willPlay in played:
        willPlay = (randrange(3), randrange(3))
    played.add(willPlay)
    print(f"{willPlay[0]},{willPlay[1]}")
    played.add(eval(input()))