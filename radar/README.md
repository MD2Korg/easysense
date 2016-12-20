# Radar Installation Instructions

## Intel Edison configuration
Components Required
1. Intel Edison - 1
2. USB OTG adapter and cable - 1
3.  USB cable - 2

## Install dependencies
```
opkg install systemd-dev libusb-1.0-dev kernel-module-ftdi-sio swig-dev coreutils-dev libpython2.7-1.0 git

```

### Install libftdi
```
wget http://www.intra2net.com/en/developer/libftdi/download/libftdi1-1.3.tar.bz2
tar -xjf libftdi1-1.3.tar.bz2
cd libftdi1-1.3

mkdir build
cd build

cmake -DCMAKE_INSTALL_PREFIX="/usr" ../
make
make install

```


### Clone EasySense repository
```
cd /home/root/
git clone git@github.com:MD2Korg/easysense.git
```

### Install libmpsse
```
cd /home/root/easysense/radar/libmpsse-1.3/src/

./configure --disable-python
make
make install
```

### EasySense Collector
```
cd /home/root/easysense/radar/
make
```

## Verify functionality
```
root@EasySense:# cd /home/root/easysense/radar/

root@EasySense:~/easysenseradar# ./easysense.sh 12345 10
Base filenames: 12345
Chip ID read is 3	6
Chip ID read is 3	6
ADC is not connected
Found both radar and ADC
Starting measurements....
Chip ID read is 3	6
root@EasySense:~# ls -ahl 12345*
-rw-r--r--  1 root root 8.0K Jun 24 15:35 12345_Radar.txt
-rw-r--r--  1 root root   19 Jun 24 15:35 12345_Radar_FPS.txt
-rw-r--r--  1 root root  20K Jun 24 15:35 12345_Radar_Final
-rw-r--r--  1 root root   10 Jun 24 15:35 12345_Radar_Switch.txt
```
