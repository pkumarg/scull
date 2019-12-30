#!/bin/bash 

display_usage() { 
    echo "Usage: ./scull_write.sh <set_of_bytes_to_write_in_one_time> <num_of_sets>" 
    echo -e "E.g. To write 4000 bytes: ./scull_write.sh 4000 1\n" 
    } 

if [  $# -ne 2 ]
    then
    display_usage
    exit 1
fi 

dd if=/dev/urandom of=/dev/scull0 bs=$1 count=$2
