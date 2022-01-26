#!/usr/bin/python3
from collections import defaultdict
from random import *
import sys

def printerr(*args, **kwargs):
	print(*args, **kwargs, file=sys.stderr)

played = defaultdict(lambda:'.')

def winner():
    if '.' != played[(0,0)] == played[(1,1)] == played[(2,2)]:
        return played[(0,0)]
    if '.' != played[(0,2)] == played[(1,1)] == played[(2,0)]:
        return played[(0,2)]
    for i in range(3):
        if '.' != played[(i,0)] == played[(i,1)] == played[(i,2)]:
            return played[(i,0)]
        if '.' != played[(0,i)] == played[(1,i)] == played[(2,i)]:
            return played[(0,i)]
    return None

me = input()
notMe = 'O'

printerr(f"smart script started as {me}\n")

if me == 'O':
    notMe = 'X'
    played[eval(input())] = notMe

priority = [
    (1,1),
    (0,0),
    (2,2),
    (0,2),
    (2,0),
    (1,2),
    (1,0),
    (0,1),
    (2,1),
]

while not winner():
    for willPlay in priority:
        if played[willPlay] == '.':
            break
    # willPlay = (randrange(3), randrange(3))
    # while willPlay in played:
    #     willPlay = (randrange(3), randrange(3))
    played[willPlay] = me
    printerr(f"{me}: playing at {willPlay}")
    print(f"{willPlay[0]},{willPlay[1]}")
    played[eval(input())] = notMe