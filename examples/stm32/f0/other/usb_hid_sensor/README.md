# README

This example implements a USB HID Sensor device. It creates a custom device
which can be found in sysfs:

```
# cd /sys/bus/platform/drivers/hid_sensor_custom/HID-SENSOR-2000e1.1.auto 
# echo 1 > enable_sensor 
# cat input-0-200544/input-0-200544-value 
```

See also: https://www.kernel.org/doc/html/latest/hid/hid-sensor.html

