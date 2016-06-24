# Radar Installation Instructions

## Intel Edison configuration
Components Required
1. Intel Edison - 1
2. USB OTG adapter and cable - 1
3.  USB cable - 2 

Accessing USB Mass storage from within Intel Edison
source: https://communities.intel.com/thread/55510?start=0&tstart=0
make the directory as mount point
mkdir /massStorage
Initialization
losetup -o 8192 /dev/loop0 /dev/disk/by-partlabel/update
mounting the partition
mount /dev/loop0 /massStorage
Unmounting the mass storage device
1. umount /massStorage
2. modprobe g_multi
Setting up the repository
source: http://alextgalileo.altervista.org/edison-package-repo-configuration-instructions.html
command: opkg install "Link"
Steps to be followed
Open the file in editor
1. vi /etc/opkg/base-feeds.conf
2. Copy the following lines:
src/gz all http://repo.opkg.net/edison/repo/all
src/gz edison http://repo.opkg.net/edison/repo/edison
src/gz core2-32 http://repo.opkg.net/edison/repo/core2-32
3. Save and exit
4. Run command opkg update.
Installing libusb
source: https://communities.intel.com/thread/55789?start=0&tstart=0
command: opkg install libusb-1.0-dev
Installing ftdi kernel
source: https://communities.intel.com/thread/62198?tstart=0
1. opkg install http://repo.opkg.net/edison/repo/edison/kernel-module-ftdi-sio_3.10.17-r0_edison.ipk
2. Will mostly show errors. Delete the file in /boot and repeat the above step
Installing Swig Dev
1. command: opkg install http://repo.opkg.net/edison/repo/core2-32/swig-dev_3.0.5-r0_core2-32.ipk
Installing core Utils required in make files sometimes
1. command: opkg install http://repo.opkg.net/edison/repo/core2-32/coreutils-dev_8.22-r0_core2-32.ipk
Install Python dev
1. command: opkg install http://repo.opkg.net/edison/repo/core2-32/libpython2.7-1.0_2.7.3-r0.3_core2-32.ipk
Install LibFtdi
1. cd /home/Libraries/libftdi1-1.1
2. mkdir build
3. cd build
4. cmake -DCMAKE_INSTALL_PREFIX="/usr" ../
5. make
6. make install\
Install libmpsse
1. cd /home/Libraries/libmpsse-1.3/src/
2. ./configure --disable-python
3. make
4. make install
Compiling our source codes
code is located in /home/code
1. cd /home/code
2. gcc -pthread -I /usr/include/libftdi1/ -I /usr/local/include/ -L /usr/lib/ -L /usr/local/lib/ collectData.c -lmpsse -lusb-1.0 -lftdi1 -o collectData.o
3. gcc -pthread -I /usr/include/libftdi1/ -I /usr/local/include/ -L /usr/lib/ -L /usr/local/lib/ ChipIdMod.c -lmpsse -lusb-1.0 -lftdi1 -o chipIdMod.o
4.  export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib/:/usr/local/lib/
