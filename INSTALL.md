# WindowsPerf Driver Installation And User-Space Application Usage

This file provides detailed instructions on how to install the WindowsPerf tool. It covers all the necessary steps, including system requirements, downloading the tool, and executing the installation process. By following the guidelines in this document, users can ensure a smooth and successful installation of WindowsPerf, enabling them to leverage its powerful performance monitoring and analysis capabilities.

> :warning: See [Known Installation Issues](#known-installation-issues) for troubleshooting.

## Prerequisites

Native Windows On Arm hardware is required for WindowsPerf to install and operate correctly.

> :warning: Please note that WindowsPerf will not work correctly (or you may not be able to install the Kernel driver) on VMs (virtual machines).

## Steps

1. **Download** the latest WindowsPerf zipped package from the [release page](https://github.com/arm-developer-tools/windowsperf/releases).
2. **Unzip** the `windowsperf-bin-x.y.z.zip` file to a local folder.
3. Navigate to the `wperf-driver` sub-directory that contains the WindowsPerf Kernel Driver files.
4. From the command line as `Administrator`, execute the command `wperf-devgen.exe install`:

```
> wperf-devgen.exe install
```

For more details, see the [Driver Installation](https://github.com/arm-developer-tools/windowsperf/blob/main/wperf-devgen/README.md#driver-installation) section in the `wperf-devgen.exe` documentation. Example:

5. After the driver is successfully installed, you can start using the `wperf.exe` user-space application. There's no need to use Administrator privileges for this.

6. To check your driver and user-space application compatibility, run the following command:

```
> wperf.exe --version
```

This command will display the version of `wperf.exe`, which can help you verify if it's compatible with the installed driver. If you encounter any issues or need further assistance, feel free to ask [here](https://github.com/arm-developer-tools/windowsperf/issues).

## Uninstalling The WindowsPerf Driver

If you no longer require the use of WindowsPerf, you have the option to uninstall the `wperf-driver` from your system. Detailed instructions for the driver uninstallation process can be found in the `Driver uninstallation` section of the [WindowsPerf documentation](https://github.com/arm-developer-tools/windowsperf/blob/main/wperf-devgen/README.md#driver-uninstallation).

Example:

```
> wperf-devgen uninstall
```

# Known Installation Issues

## wperf-devgen.exe - Application Error

If you see error:

> :warning: "The application was unable to start correctly (0xc000007b). Click OK to close this application."

when you issue e.g. `wperf-devgen.exe` command please refer to [A Windows Error 0xC000007B](https://answers.microsoft.com/en-us/windows/forum/all/a-windows-error-0xc000007b/c25fbcb7-1162-487f-bd81-6d8314a52891) article.
Usually errors like these are due to missing `vcredist` files. We suggest downloading the updated ones from Microsoft's page, for example [Latest Microsoft Visual C++ Redistributable Version](https://learn.microsoft.com/en-US/cpp/windows/latest-supported-vc-redist?view=msvc-170#latest-microsoft-visual-c-redistributable-version).
