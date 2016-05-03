# EasySense Procotol and System Configuration

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
