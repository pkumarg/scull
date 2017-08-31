#!/bin/bash 

display_usage() { 
    echo "Usage: ./scull_write.sh <output_device(file)_name> <set_of_bytes_to_write_in_one_time> <num_of_sets>" 
    echo -e "E.g. To write 4000 bytes: ./scull_write.sh /dev/scull0 4000 1\n" 
    } 

if [  $# -le 2 ]
    then
    display_usage
    exit 1
fi 

dd if=/dev/urandom of=$1 bs=$2 count=$3
