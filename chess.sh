#!/bin/bash

cd /home/hainh/Desktop/Project/ChessBot/ChessPlayer/build

# Define the process name and path
PROCESS_NAME="ChessPlayer"
EXEC_PATH="./ChessPlayer"

# Generate the log file name with the current date
LOG_DIR="./"

# Ensure the log directory exists
mkdir -p "$LOG_DIR"

echo "$(date '+%Y-%m-%d %H:%M:%S') - Monitoring loop started for $PROCESS_NAME." >> "${LOG_DIR}/console_log_$(date +%Y-%m-%d).txt"

# Infinite loop to keep checking the process
while true
do
    # Dynamically update log file name so it rolls over at midnight
    LOG_FILE="${LOG_DIR}/console_log_$(date +%Y-%m-%d).txt"

    # Check if the process is running
    if ! pgrep -x "$PROCESS_NAME" > /dev/null
    then
        echo "$(date '+%Y-%m-%d %H:%M:%S') - CRASH DETECTED: $PROCESS_NAME is not running. Restarting..." >> "$LOG_FILE"
        
        # Execute in foreground inside the loop so the loop waits until it exits/crashes
        # This captures both standard output and errors
        $EXEC_PATH >> "$LOG_FILE" 2>&1
        
        echo "$(date '+%Y-%m-%d %H:%M:%S') - $PROCESS_NAME terminated." >> "$LOG_FILE"
    fi

    # Wait 5 seconds before checking again to prevent high CPU usage if it rapidly restarts
    sleep 5
done

