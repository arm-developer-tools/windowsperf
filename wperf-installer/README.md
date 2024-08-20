# WindowsPerf Installer

[[_TOC_]]

## Introduction

This project uses [WiX Toolset](https://wixtoolset.org/) to build a MSI package to install WindowsPerf. This project requires WiX v5. Make sure to download and install, you should be able to create a new WiX project on Visual Studio if everything was done correctly.

## Building

The variable `ReleaseFolder` controls the path of the current release that is going to be packaged in the installer. The expected folder structure is the same one from releases after ETW. Under `wperf-installer\bin\ARM64\$(Configuration)\en-US\` you will find the MSI package.

## Considerations

The projects are not added as dependencies so they need to be built first. This was done because the signing process for the driver is manual. One will also notice that there are custom actions for uninstalling, this is because WiX does not allow you to repeat the action with two different parameters, during uninstall it had to be done before `InstallFinalize` and during installation it should be done after.
