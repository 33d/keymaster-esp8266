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
[replacement-inductor-rs]: http://au.rs-online.com/web/p/multilayer-surface-mount-inductors/6041524/
