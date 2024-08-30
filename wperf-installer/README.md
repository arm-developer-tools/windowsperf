# WindowsPerf Installer

[[_TOC_]]

## Introduction

This project uses [WiX Toolset](https://wixtoolset.org/) to build a MSI package to install WindowsPerf. This project requires WiX v5. Make sure to download and install, you should be able to create a new WiX project on Visual Studio if everything was done correctly.

## Environment Variables

When you run the WindowsPerf installer, it will automatically add `wperf.exe` to your system’s `PATH`, making it easily accessible from any command prompt. Additionally, the installer will set up a new environment variable called `WINDOWSPERF_PATH`, which will contain the root installation directory of WindowsPerf. This ensures that all necessary paths and configurations are correctly set up.

In addition to setting up `wperf.exe` and `WINDOWSPERF_PATH`, the WindowsPerf installer will also update the `WPA_ADDITIONAL_SEARCH_DIRECTORIES` environment variable. This variable will be configured to include the folder containing WindowsPerf WPA plugins, ensuring that these plugins are easily accessible and integrated into your workflow.

Note: The `WPA_ADDITIONAL_SEARCH_DIRECTORIES` environment variable is used to specify additional directories where Windows Performance Analyzer (WPA) should look for plugins.

## Building

The variable `ReleaseFolder` controls the path of the current release that is going to be packaged in the installer. The expected folder structure is the same one from releases after ETW. Under `wperf-installer\bin\ARM64\$(Configuration)\en-US\` you will find the MSI package.

## Considerations

The projects are not added as dependencies so they need to be built first. This was done because the signing process for the driver is manual. One will also notice that there are custom actions for uninstalling, this is because WiX does not allow you to repeat the action with two different parameters, during uninstall it had to be done before `InstallFinalize` and during installation it should be done after.
