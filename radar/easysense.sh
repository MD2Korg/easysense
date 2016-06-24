#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib/:/usr/local/lib
cd /home/root/easysense/radar/
./collectData $1 $2 > /home/root/easysense.log 2>/home/root/easysense_error.log
