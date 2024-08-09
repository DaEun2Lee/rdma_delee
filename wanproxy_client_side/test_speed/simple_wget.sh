#!/bin/bash

#for i in {1..100}; do
  echo "Running iteration $i"    
start_time=$(date +%s.%N)    
#  sudo sshpass -f ./passwd_ubuntu.txt scp -P 3300 ubuntu@10.20.16.66:/home/ubuntu/test/Bible.pdf .    
wget 10.20.16.78:80/index.html
end_time=$(date +%s.%N)  
execution_time=$(echo "$end_time - $start_time" | bc)    
  file_size=$(du -h index.html | cut -f1)    
echo "Result for iteration $i: Exit code: $? | Execution time: $execution_time seconds | File size: $file_size" >> results.log
#done
