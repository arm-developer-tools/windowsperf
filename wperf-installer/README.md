# WindowsPerf Installer

This project utilizes the [WiX Toolset](https://wixtoolset.org/) to build an MSI package for installing WindowsPerf. It requires WiX v5, so ensure you download and install it. If everything is set up correctly, you should be able to create a new WiX project in Visual Studio.

# Project wperf-installer Dependencies

You may need additional software to load and build `wperf-installer` project based on [WiX Toolset](https://wixtoolset.org/). In short you may need a `Wix.exe .NET tool` and `WiX v3 - Visual Studio 2022 Extension`. WiX is available as a [.NET tool](https://learn.microsoft.com/en-us/dotnet/core/tools/global-tools).

> :warning: The `wix.exe` tool requires the .NET SDK, version 6 or later.

- To install the `Wix.exe .NET tool`:

```
> dotnet tool install --global wix
```

- To install `WiX v3 - Visual Studio 2022 Extension` go to Visual Studio Marketplace [here](https://marketplace.visualstudio.com/items?itemName=WixToolset.WixToolsetVisualStudio2022Extension).

Now you may 

> :warning: Please refer to [Get started with WiX](https://wixtoolset.org/docs/intro/) for more details.

# WindowsPerf MSI Installer Properties

The `wperf-installer` project is responsible for generating the MSI WindowsPerf installer. This installer is crucial for deploying the WindowsPerf tool.

The MSI (Microsoft Software Installer) format is used by Windows to install, maintain, and remove software. It simplifies the installation process by packaging all necessary files and instructions into a single file, ensuring that the software is correctly configured and ready to use. The MSI installer for WindowsPerf deploys the kernel driver, adds the WindowsPerf CLI to the system PATH, and installs the WindowsPerf WPA-plugins.

## Environment Variables Setting

When you run the WindowsPerf installer, it will automatically add `wperf.exe` to your system’s `PATH`, making it easily accessible from any command prompt. Additionally, the installer will set up a new environment variable called `WINDOWSPERF_PATH`, which will contain the root installation directory of WindowsPerf. This ensures that all necessary paths and configurations are correctly set up.

In addition to setting up `wperf.exe` and `WINDOWSPERF_PATH`, the WindowsPerf installer will also update the `WPA_ADDITIONAL_SEARCH_DIRECTORIES` environment variable. This variable will be configured to include the folder containing WindowsPerf WPA plugins, ensuring that these plugins are easily accessible and integrated into your workflow.

Note: The `WPA_ADDITIONAL_SEARCH_DIRECTORIES` environment variable is used to specify additional directories where Windows Performance Analyzer (WPA) should look for plugins.

## Building

The variable `ReleaseFolder` controls the path of the current release that is going to be packaged in the installer. The expected folder structure is the same one from releases after ETW. Under `wperf-installer\bin\ARM64\$(Configuration)\en-US\` you will find the MSI package.

## Considerations

The projects are not added as dependencies so they need to be built first. This was done because the signing process for the driver is manual. One will also notice that there are custom actions for uninstalling, this is because WiX does not allow you to repeat the action with two different parameters, during uninstall it had to be done before `InstallFinalize` and during installation it should be done after.
