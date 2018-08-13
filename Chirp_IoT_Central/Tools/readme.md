# Chirp and Azure IoT Central Tools

Tooling to create a drag and drop version of compiled .ino sketch for the MXChip

## Contents

Run using the following command line from the Tools directory:

```Bash
python genDragnDropBin.py <chirpFirmware.ino.bin> <chirpFirmware.bin> 
```
<chirpFirmware.ino.bin> can be found in the build directory after doing an Arduino:Verify or Arduino:Upload command in Visual Studio Code.  The default placement of the build directory is dependent on the "output" value in the ./.vscode/arduino.json file in the root of the repository.  For example if the arduino.json file looks like this:

```JSON
{
    "board": "AZ3166:stm32f4:MXCHIP_AZ3166",
    "configuration": "upload_method=OpenOCDMethod",
    "sketch": "chirpFirmware/chirpFirmware.ino",
    "port": "/dev/cu.usbmodem14303",
    "output": "./build"
}
```

Then the <chirpFirmware.ino.bin> file should be replaced with ../build/chirpFirmware.ino.bin .  The <chirpFirmware.bin> is the output file for the drag-and-drop bin file and can be whatever you choose.  For example: ../chirpFirmware.bin

The boot.bin version is the 1.4.1 variant of the boot file.  You can also point at the boot file in the Arduino15 directory here 
    __MacOS__: /Users/{user name}/Library/Arduino15/packages/AZ3166/hardware/stm32f4/1.4.1/bootloader  
    __Windows__: C:\users\{user name}\AppData\local\Arduino15\packages\AZ3166\hardware\stm32f4\1.4.1\bootloader

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
