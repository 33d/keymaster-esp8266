# Building

*This probably won't work on Windows, unless you use msys2 or Cygwin.*

 1. Install Arduino
 1. Install the "git version" of the esp8266 Arduino toolchain (I used version 2.4.0-rc2, 0c897c3), following [the instructions in the README][esp8266-arduino-install-git].
 1. Run `make TOOL_HOME=<arduino-home>/hardware/esp8266com/esp8266` from this project's root directory.

## PlatformIO

**This procedure doesn't work yet.**  It produces this message:

    .pioenvs/nodemcuv2/firmware.elf section \`.text' will not fit in region \`iram1_0_seg'

This seems to be caused by the object files ending in `.o` instead of `.cpp.o` (see the linker script `eagle.app.v6.common.ld`).

This won't build with the stock PlatformIO ESP8266 Arduino toolchain, because version 3.2.0 doesn't handle verifying TLS keys.  To fix this:

1. Check out the latest [Arduino toolchain][esp8266-arduino], copy the contents to `~/.platformio/packages/framework-arduinoespressif8266` (make a copy of your old directory first)
1. In the above directory:
  1. Create `version.txt`, put in it: `1.20300.1`
  1. Create `package.json`, put in it:

     ```
     {
         "description":"Arduino Wiring-based Framework (ESP8266 Core)",
         "name":"framework-arduinoespressif8266",
         "system":"all",
         "url":"https://github.com/esp8266/Arduino",
         "version":"1.20300.1"
     }
     ```

  1. Run `pio platform show espressif8266`; it should show (note `Installed: Yes`):

     ```
     Package framework-arduinoespressif8266
     --------------------------------------
     Type: framework
     Requirements: ~1.20300.1
     Installed: Yes
     Description: Arduino Wiring-based Framework (ESP8266 Core)
     Url: https://github.com/esp8266/Arduino
     Version: 1.20300.1 (2.3.0)
     ```  

  1. Run `pio run`; it shouldn't download a new Arduino toolchain.
 1. Edit `~/.platformio/platforms/espressif8266/builder/main.py`, comment out `"-Wl,-wrap,register_chipv6_phy"`

Resources:

* [Using root certificates][use-root-certificate]
* [Adding a new framework to PlatformIO][platformio-new-framework]
* [Using the development version of a framework][platformio-development-framework]

# Wiring

To connect a NodeMCU 1.0 to the RFID-RC522:

| ESP8266-12E | NodeMCU | RFID-RC522 |
|:-----------:|:-------:|:----------:|
| GPIO12      | D6      | MISO       |
| GPIO13      | D7      | MOSI       |
| GPIO14      | D5      | SCK        |
| GPIO04      | D2      | SDA        |
| GPIO05      | D1      | RST        |

# Fixing poor range

The cheap "RFID-RC522" boards [have inductors which can't handle the transmit current][replace-inductor].  Replacing the inductors L1 and L2 with [2.2µH, 150mA][replacement-inductor-rs] ones increases the range to a few centimetres.

# Other projects

* [esp8266-mfrc522](https://github.com/Jorgen-VikingGod/ESP8266-MFRC522)

[replace-inductor]: https://forum.mikroe.com/viewtopic.php?p=255809#p255809
[replacement-inductor-rs]: http://au.rs-online.com/web/p/multilayer-surface-mount-inductors/6041524
[esp8266-arduino]: https://github.com/esp8266/Arduino
[use-root-certificate]: https://github.com/esp8266/Arduino/pull/2968
[platformio-new-framework]: https://community.platformio.org/t/adding-a-new-framework-to-platformio/297
[platformio-development-framework]: https://github.com/platformio/platformio-core/issues/339#issuecomment-160164467
[esp8266-arduino-install-git]: https://github.com/esp8266/Arduino/blob/f30c03b9e132914b253f3d6de69c80a260226173/README.md#using-git-version
