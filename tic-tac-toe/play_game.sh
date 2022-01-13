#!/bin/bash

pipes={in,out}{1,2}

echo {in,out}{1,2}

mkfifo {in,out}{1,2}

# tmux split-window './rw in2 out2'
tmux split-window 'python3 random_player.py < in1 > out1; read'
tmux split-window './rw in2 out2'

./game < game_input

echo "script done!"

rm {in,out}{1,2}