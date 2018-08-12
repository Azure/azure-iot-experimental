# Chirp and Azure IoT Central Sample Code

Reference firmware implementation for using Chirp with Azure IoT Central. Plus a web browser based provisioning sample application to quickly get MXChip devices onto Azure IoT Central.

## Contents

The sub-directories contain the following content:

* [chirpFirmware](chirpFirmware) - MXChip firmware source for building with Visual Studio Code and the Arduino plugin.
* [Chirp_MXChip_SDK](Chirp_MXChip_SDK) - Include and library files for the MXChip SDK along with the platform.local.txt for compiling with the Chirp SDK library
* [prebuilt_firmware](prebuilt_firmware) - Pre-built firmware for the MXChip that can be drag-and-dropped onto the device.
* [chirpweb](chirpweb) - Web browser application to provision MXChip devices onto Azure IoT Central. 
* [cleanEEPROM](cleanEEPROM) - Code for cleaning a corrupted EEPROM on the MXChip
* [Tools](Tools) - Miscelaneous tooling including the python script to build a drag-n-drop firmware .bin

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
