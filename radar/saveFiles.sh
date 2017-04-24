#!/bin/bash
cp -R /home/root/Data/* /media/sdcard/
sudo umount /media/sdcard 
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib:/usr/lib
./buzzer
