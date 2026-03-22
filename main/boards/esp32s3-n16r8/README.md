# ESP32-WROOM-32
Custom board profile for ESP32-WROOM-32 with SPI LCD + I2S mic/speaker.

## Build (ESP-IDF)
1. Open ESP-IDF PowerShell.
2. `cd` to project root.
3. Run `idf.py set-target esp32`.
4. Run `idf.py menuconfig`.
5. Go to `Xiaozhi Assistant -> Board Type -> ESP32-WROOM-32 (Custom SPI LCD + I2S)`.
6. Set flash size/partition according to your module (for 4MB, use `partitions/v2/4m.csv`).
7. Save and exit.
8. Build with `idf.py build`.

## Flash
1. Connect board by USB.
2. Flash with `idf.py -p COM10 flash` (replace COM port if needed).
3. Monitor with `idf.py -p COM10 monitor`.
