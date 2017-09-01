# README

UART TX on PA2 @ 115200/8n1

This example reads the onboard accelerometer and dumps the raw value
of the ACC_OUT_X_L_A/ACC_OUT_X_H_A registers.  (you should see ~0 for flat,
and positive/negative values for tipping the board along it's long axis,
ranging up to plus/minus 16k or so for vertical.
