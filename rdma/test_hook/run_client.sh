#!/bin/bash

# Log file
LOGFILE="times.log"

# Remove existing log file if it exists
if [ -f $LOGFILE ]; then
    rm $LOGFILE
fi

# Execute the client 100 times
for i in {1..100}
do
    START=$(date +%s%3N)
    ./client
    END=$(date +%s%3N)
    DURATION=$((END-START))
    echo "Execution $i: $DURATION ms" >> $LOGFILE
done

echo "All executions are completed. Check $LOGFILE for execution times."
