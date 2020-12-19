# README

This example implements a USB MIDI device to demonstrate the use of the
USB device stack. It implements the device configuration found in Appendix
B of the Universal Serial Bus Device Class Definition for MIDI Devices.

The 'USER' button sends note on/note off messages.

The board will also react to identity request (or any other data sent to
the board) by transmitting an identity message in reply.

## Board connections

| Port  | Function       | Description                               |
| ----- | -------------- | ----------------------------------------- |
| `CN5` | `(USB_OTG_FS)` | USB acting as device, connect to computer |

## Testing

To list midi devices, which should include this demo device

    $ amidi -l
    Dir Device    Name
    IO  hw:2,0,0  MIDI demo MIDI 1
    $

To record events, while pushing the user button

    $ amidi -d -p hw:2,0,0
    
    90 3C 40   -- key down
    80 3C 40   -- key up
    90 3C 40
    80 3C 40^C
    12 bytes read
    $

To query the system identity, note this dump matches sysex\_identity[] in the
source.

    $ amidi -d -p hw:2,0,0 -s Sysexdump.syx
    
    F0 7E 00 7D 66 66 51 19 00 00 01 00 F7
    ^C
    13 bytes read
    $

