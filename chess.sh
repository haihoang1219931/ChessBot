#!/bin/bash

cd /home/hainh/Desktop/Project/ChessBot/ChessPlayer/build
# Define the process name and path
PROCESS_NAME="ChessPlayer"
EXEC_PATH="./ChessPlayer"

# Generate the log file name with the current date
LOG_DIR="./"
LOG_FILE="${LOG_DIR}/console_log_$(date +%Y-%m-%d).txt"

# Ensure the log directory exists
mkdir -p "$LOG_DIR"

# Check if the process is running
if ! pgrep -x "$PROCESS_NAME" > /dev/null
then
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $PROCESS_NAME is not running. Starting it now..." >> "$LOG_FILE"
    
    # Execute and redirect the process's own output to the log file
    $EXEC_PATH >> "$LOG_FILE" 2>&1 &
else
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $PROCESS_NAME is already running." >> "$LOG_FILE"
fi

