# Install the firmware

Download the image for your board from the
[`latest` pre-release](https://github.com/stan-kondrat/jsbedded/releases/tag/latest)
(rebuilt on every push to `main`) or a tagged release. Check it against `SHA256SUMS`.

Images are built per board and engine; `none` has no JavaScript. Today they only
print a hello line once a second, to prove the build and flashing path.

**Pico family:** hold BOOTSEL, plug in USB, copy `jsbedded-<board>-<engine>.uf2` to the
drive that appears. Output: the USB serial port.

**ESP32-S3 / ESP32-C3:** connect the dev board's USB-UART port, then

```bash
esptool.py write_flash 0x0 jsbedded-<board>-<engine>.bin
```

Output: the same serial port at 115200 baud.
