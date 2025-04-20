#!/bin/bash

SESSION_NAME="mysession"

# Start new session and window
tmux new-session -d -s "$SESSION_NAME" -n "main"
tmux set-option -t "$SESSION_NAME" -g mouse on

# Split the window horizontally (Pane 0 and Pane 1)
tmux split-window -h -t "$SESSION_NAME":0

# Split the left pane vertically (now Pane 0 becomes Pane 2, Pane 1 stays)
tmux select-pane -t "$SESSION_NAME":0.0
tmux split-window -v -t "$SESSION_NAME":0

# Optional: select the first pane again
tmux select-pane -t "$SESSION_NAME":0.0

# Attach to session
tmux attach-session -t "$SESSION_NAME"