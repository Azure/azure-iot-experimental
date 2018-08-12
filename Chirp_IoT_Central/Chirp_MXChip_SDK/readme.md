# Chirp MXChip SDK Files

Files for the Chirp MXChip SDK

## Contents

After installing the MXChip SDK from [here](https://microsoft.github.io/azure-iot-developer-kit/docs/get-started/).

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

A Chirp provided sample can be found in the Chirp-sample directory.

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
