# Chirp and Azure IoT Central pre-built Firmware

Reference firmware implementation for using Chirp with Azure IoT Central.

## Contents

A pre-built firmware binary that can be dragged and dropped onto the Azure MXChip device without the need to build the source code.

## How to use

Download the chirpFirmware.bin file.  Connect the MXChip device to your computer, a drive named AZ3166 should be added to the computer.  Drag and drop the chirpFirmware.bin file onto the drive, the firmware will be installed and the device will reboot.  After rebooting the device should be in listening mode and ready to be Chirped at.  See the readme.md in the chirpweb directory for details how to Chirp to your device and add it to IoT Central.

If the EEPROM on the device contains residual data the Chirp firmware may not work correctly and interpret the values in the EEPROM as the WiFi credentials or the connection string.  If the firmware does not work as expected drag and drop the cleanEEPROM.bin file onto the AZ3166 drive and let it load and run.  This will clean out the values in the EEPROM.  Now reload the chirpFirmware.bin as described above.

## Status

This project is provided to illustrate how you can utilize the Chirp SDK on MXChip to provision devices to services like Azure IoT Central using data transmission over sound.  This project is not designed to be production ready but provided as a sample of how to build the Chirp feature into your firmware.

## Reporting Security Issues

Security issues and bugs should be reported privately, via email, to the Microsoft Security
Response Center (MSRC) at [secure@microsoft.com](mailto:secure@microsoft.com). You should
receive a response within 24 hours. If for some reason you do not, please follow up via
email to ensure we received your original message. Further information, including the
[MSRC PGP](https://technet.microsoft.com/en-us/security/dn606155) key, can be found in
the [Security TechCenter](https://technet.microsoft.com/en-us/security/default).

## Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.