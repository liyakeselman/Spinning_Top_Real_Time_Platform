#!/bin/bash
#chmod +x bist_integrated_real_time.sh

ESP_IP_ADDRESS="192.168.65.163" # change it according to your ESP_IP_ADDRESS 
FILE="sensors_data.csv"
TEMP_FILE="./tmp_file.csv"
LOG_FILE="./realtime_updates.log"

# Function to download the file
download_file() {
    curl -s -o "$FILE" "http://$ESP_IP_ADDRESS/$FILE"
    if [ $? -ne 0 ]; then
        echo "Error downloading initial file."
        exit 1
    fi
}


# Clear TEMP_FILE and LOG_FILE
rm -f $TEMP_FILE $LOG_FILE

# Initial download of the file
echo "Downloading initial file ..."
download_file


# If TEMP_FILE does not exist, create it
if [ ! -f $TEMP_FILE ]; then
    cp $FILE $TEMP_FILE
    if [ $? -ne 0 ]; then
        echo "Error creating temporary file."
        exit 1
    fi
fi


# Monitor for new lines
while true; do
    # Fetch the latest version of the file
    echo "Fetching latest file..."
    download_file # Fetch the latest version of the file
    if [ $? -ne 0 ]; then
        echo "Error fetching the file."
        continue
    fi

    # Compare the previous state with the current file
    new_lines=$(diff $TEMP_FILE $FILE | grep '^> ' | sed 's/^> //')

    # Print new lines
    if [ ! -z "$new_lines" ]; then
        echo "$new_lines" >> $LOG_FILE  # Append to log file
        echo "$new_lines" # Display in terminal
    fi

    # Update the temporary file
    cp $FILE $TEMP_FILE
    if [ $? -ne 0 ]; then
        echo "Error updating temporary file."
        continue
    fi

    sleep 1 # Adjust the delay as needed
done