#!/usr/bin/python3
from random import *
import sys

def printerr(*args, **kwargs):
	print(*args, **kwargs, file=sys.stderr)

printerr("player script started!\n")
played = set()
me = input()

printerr(f"i am {me}!")

if me == 'O':
    played.add(eval(input()))

while True:
    willPlay = (randrange(3), randrange(3))
    while willPlay in played:
        willPlay = (randrange(3), randrange(3))
    played.add(willPlay)
    printerr(f"{me}: playing at {willPlay}")
    print(f"{willPlay[0]},{willPlay[1]}")
    played.add(eval(input()))