# Chirp and Azure IoT Central Firmware Source

Reference firmware source code for using Chirp with Azure IoT Central.

## Contents

The source code for the pre-built firmware binary.  The code is in a single Arduino compatible .ino file and does not require any external libraries other than those for the Chirp SDK.  The code can be built on Windows 10, MacOS, or Linux using Visual Studio Code, Arduino IDE, MXChip SDK, and the Chirp SDK files provided in this repository.

## How to use

Clone the repository or download the directory onto your computer.  Install [Visual Studio Code](https://code.visualstudio.com/download) and [Arduino IDE](https://www.arduino.cc/en/Main/Software) then follow the instructions for [installing the MXChip SDK](https://microsoft.github.io/azure-iot-developer-kit/docs/get-started/).

Install the Chirp SDK.  Unfortunately this is a manual step currently, we hope to streamline this in the near future.  For now follow the following steps:

* Create a directory named Chirp in the directory:

  __MacOS__: ~/Library/Arduino15/packages/AZ3166/hardware/stm32f4/1.4.1/libraries  
  __Windows__: %LOCALAPPDATA%\Arduino15\packages\AZ3166\hardware\stm32f4\1.4.1\libraries
* Copy all the .h files from the include directory (chirp_connect_callbacks.h, chirp_connect_errors.h, chirp_connect_states.h, chirp_connect.h, chirp_sdk_defines.h) into the created Chirp directory
* Copy the library file in the lib (.za) directory (libchirp-connect_none-armv7m-cm4-softfp.a )into the directory:
  
  __MacOS__: ~/Library/Arduino15/packages/AZ3166/hardware/stm32f4/1.4.1/system  
  __Windows__: %LOCALAPPDATA%\Arduino15\packages\AZ3166\hardware\stm32f4\1.4.1\system
* Copy the platform.local.txt file into the directory:
  
  __MacOS__: ~/Library/Arduino15/packages/AZ3166/hardware/stm32f4/1.4.1  
  __Windows__: %LOCALAPPDATA%\Arduino15\packages\AZ3166\hardware\stm32f4\1.4.1

Load the chirpFirmware.ino file into the Visual Studio Code editor and go to line 241.

```C
// these need to be defined by the user from https://developer.chirp.io
#define APP_KEY "<replace with application key from Chirp>"
#define APP_SECRET "<replace with application secret from Chirp>"
#define APP_LICENCE "<replace with licence key from Chirp>"
```

Go to [Chirp developer web site](https://developers.chirp.io) sign up and get an application key and secret pair and aquire a Microsoft-MXChip protocol licence.  Download the licence key and cut and paste the file contents into the APP_LICENCE #define.

Plug the MXChip device into your computer and configure Visual Studio so it can communicate with the device, see instructions [here](https://microsoft.github.io/azure-iot-developer-kit/docs/get-started/) if not already completed.  Compile the code for upload to the device with CTRL-SHIFT-P (COMMAND-SHIFT-P on MacOS) and select Arduino: Upload.  The source will compile and be uploaded to the attached MXChip device.

Once uploaded the device will reboot and enter listening mode waiting for your first Chirp.  See the readme.md in the chirpweb directory for details how to Chirp to your device and add it to IoT Central.

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
