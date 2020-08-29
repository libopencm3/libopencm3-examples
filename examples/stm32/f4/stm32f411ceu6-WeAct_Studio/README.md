# WeAct Studio STM32F411CEU6 Core Board

Example code for STM32F411CEU6 Core Board from WeAct Studio:

https://github.com/WeActTC/MiniF4-STM32F4x1

## Programming

### Using the system DFU (dfu-util)

These devices comes with a USB DFU bootloader baked at the ROM that you can use for programming:

- Connect the USB cable.
- Press and **hold** the `BOOT0` button.
- Press and **release** the `NRST` button.
- **Release** the `BOOT0` button.

The Linux kernel should recognize and attach the device:

```
[ 2042.399489] usb 7-2: new full-speed USB device number 14 using xhci_hcd
[ 2042.574420] usb 7-2: New USB device found, idVendor=0483, idProduct=df11, bcdDevice=22.00
[ 2042.574423] usb 7-2: New USB device strings: Mfr=1, Product=2, SerialNumber=3
[ 2042.574424] usb 7-2: Product: STM32  BOOTLOADER
[ 2042.574425] usb 7-2: Manufacturer: STMicroelectronics
[ 2042.574426] usb 7-2: SerialNumber: 355437763437
```

Then program it with:

```
make dfu_download
```

**Important:** The `--reset` option from `dfu-util` does **not** work: you **must** push `NRST` to reset the device after downloading.

### ST-LINK V2 (openocd)

You can use the [ST-LINK V2 in-circuit debugger/programmer](https://www.st.com/en/development-tools/st-link-v2.html) (or one of the [inexpensive](https://www.aliexpress.com/wholesale?catId=0&initiative_id=SB_20200829033204&SearchText=ST-LINK+V2) options) to program it:

Hook the ST-LINK V2 to the board:

- `GND` > `GND`
- `SWCLK` > `SCK`
- `SWDIO` > `DIO`
- `RST` > `R`
- `3.3V` > `3V3`
  - Beware of ST-LINK V2's 3.3V regulator current limit.
  - You can skip connecting this and power the board from though the USB-C connector instead.

Program it with:

```
make openocd_program
```

## Debugging

Hook up ST-LINK V2 as described above then, in one terminal do:

```
make openocd
```

This will start openocd listening at a TCP socket. From **another** terminal, do:

```
make gdb
```