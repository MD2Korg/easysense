# EasySense Protocol and System Configuration


# Protocol

Devices are uniquely identified by their device UUID. e.g. `FFC84E54-013D-40D9-B7C4-DFB356C0F6A9`.

| Service Name | Number | Description | Characteristic | Number | Properties |
|--------------|--------|-------------|----------------|--------|------------|
| Battery      | 0x180F | The Battery Service exposes the Battery State and Battery Level of a single battery or set of batteries in a device. | Battery Level | 0x2A19 | Read |
| EasySense | ef1a10d8-272d-4822-b436-39f87205bbac | Controller service for the EasySense board | Controller | 850a75ab-a811-405d-9817-2867ac36aafc | Write, Notify |

### BLE Protocol
EasySense will advertise itself as a peripheral `easysense` with single service `ef1a10d8-272d-4822-b436-39f87205bbac`.  This service contains a single characteristic `850a75ab-a811-405d-9817-2867ac36aafc` for controlling the sensor.  

The characteristic supports `write` and `notify` properties which allow an application to subscribe to the data stream to receive progress updates as well as return codes.  

To initiate a sample, these parameters are encoded as bytes and written to the service:
- Sampling time in seconds (1-60)
- Epoch timestamp of the reading to be recorded (32-bit unsigned int)

Example to record `60` seconds of data on `Tue, 03 May 2016 15:36:02 GMT`, write `0x3C5728C562` to the characteristic.

Data will be written to the subscribed characteristic each second to indicate the number of seconds the recording has been occuring.  Additionally, a set of codes will be written at the completion of a sample:
- SUCCESS: 250
- VERIFIED: 249 (currently unused)
- FAILURE: 248
- INPUT TO LARGE: 247
- INPUT TO SMALL: 246

### System Configuration

```
rfkill unblock bluetooth
killall bluetoothd
hciconfig hci0 up 
```

Disable Bluetooth service
```
systemctl disable bluetooth
```

```
$ cat /etc/systemd/system/rfkill-unblock.service
[Unit]
Description=RFKill-Unblock All Devices

[Service]
Type=oneshot
ExecStart=/usr/sbin/rfkill unblock all
ExecStop=
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
```

Run `systemctl enable rfkill-unblock.service`


```
$ cat /etc/ld.so.conf
/usr/lib/
/usr/local/lib
```

Run `ldconfig`



```
npm install nodejs-npm
cd /node_app_slot
npm install bluetooth-hci-socket