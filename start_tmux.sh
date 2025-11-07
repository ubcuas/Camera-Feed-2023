#!/bin/bash

SESSION_NAME="cam"

# Check and move files/directories if they exist
timestamp=$(date +"%Y%m%d_%H%M%S")
dest_dir="data/$timestamp"
mkdir -p "$dest_dir"

[ -f tag.txt ] && mv tag.txt "$dest_dir/"
[ -f detect.csv ] && mv detect.csv "$dest_dir/"
[ -d images ] && mv images "$dest_dir/"

# Start new session and window
tmux new-session -d -s "$SESSION_NAME" -n "main"
tmux set-option -t "$SESSION_NAME" -g mouse on

# Split the window horizontally (Pane 0 and Pane 1)
tmux split-window -h -t "$SESSION_NAME":0

# Split the left pane vertically (now Pane 0 becomes Pane 2, Pane 1 stays)
tmux select-pane -t "$SESSION_NAME":0.0
tmux split-window -v -t "$SESSION_NAME":0

tmux send-keys -t "$SESSION_NAME":0.0 "./camerafeed -s 3600 -b -w -t -p -e 360 -g 27"
touch tag.txt
tmux send-keys -t "$SESSION_NAME":0.1 "tail -n +1 -f tag.txt"
touch detect.csv
tmux send-keys -t "$SESSION_NAME":0.2 "tail -n +1 -f detect.csv"

# Attach to session
tmux attach-session -t "$SESSION_NAME"