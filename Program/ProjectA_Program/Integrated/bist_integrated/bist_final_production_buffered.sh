#!/bin/bash
#chmod +x bist_integrated_buffered.sh

ESP_IP_ADDRESS="192.168.65.163" # Change according to your ESP IP Address 
FILE="sensors_data.csv"
TEMP_FILE="./tmp_file.csv"
LOG_FILE="./realtime_updates.log"
CHECK_INTERVAL=1  # seconds
NO_UPDATE_TIMEOUT=50  # seconds before script ends due to no updates

# Function to download the file with error handling
download_file() {
    local attempt=0
    local max_attempts=10
    local delay=5
    while [ $attempt -lt $max_attempts ]; do
        curl -s -o "$FILE" "http://$ESP_IP_ADDRESS/$FILE"
        if [ $? -eq 0 ]; then
            return 0  # success
        else
            echo "Download attempt $((attempt + 1)) failed. Retrying in $delay seconds..."
            sleep $delay
            ((attempt++))
        fi
    done
    echo "Error downloading the file after $max_attempts attempts."
    return 1  # failure
}

# Clear TEMP_FILE and LOG_FILE
rm -f $TEMP_FILE $LOG_FILE

# Initial download of the file
echo "Downloading initial file ..."
if ! download_file; then
    echo "Initial download failed. Exiting."
    exit 1
fi

# If TEMP_FILE does not exist, create it
if [ ! -f $TEMP_FILE ]; then
    cp $FILE $TEMP_FILE
    if [ $? -ne 0 ]; then
        echo "Error creating temporary file."
        exit 1
    fi
fi

# Variables for tracking time without updates
last_update_time=$(date +%s)
current_time=0

# Monitor for new lines
while true; do
    # Fetch the latest version of the file
    echo "waiting for updates ..."
    if ! download_file; then
        echo "Error : fetching the file."
        continue
    fi

    # Compare the previous state with the current file to find new lines
    new_lines=$(diff <(sort $TEMP_FILE) <(sort $FILE) | grep '^>' | sed 's/^> //')

    # If there are new lines, update the last update time
    if [ ! -z "$new_lines" ]; then
        echo "$new_lines" >> $LOG_FILE  # append to log file
        echo "$new_lines" # display new lines in terminal
        last_update_time=$(date +%s)
    fi

    # Update the temporary file
    cp $FILE $TEMP_FILE
    if [ $? -ne 0 ]; then
        echo "Error updating temporary file."
        continue
    fi

    # Check if no updates for too long
    current_time=$(date +%s)
    if [ $((current_time - last_update_time)) -ge $NO_UPDATE_TIMEOUT ]; then
        echo "No updates for $NO_UPDATE_TIMEOUT seconds, cleaning up and ending script."
        rm -f $LOG_FILE $TEMP_FILE
        exit 0
    fi

    sleep $CHECK_INTERVAL # adjust the delay as needed
done