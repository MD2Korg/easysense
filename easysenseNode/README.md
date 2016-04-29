

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