# WindowsPerf Driver Installation and User-Space Application Usage

## Prerequisites

Native Windows On Arm hardware is required. Please note that virtual machines (VMs) will not work.

## Steps

1. **Download** the latest WindowsPerf zipped package from the [release page](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/releases).

2. **Unzip** the `windowsperf-bin-x.y.z.zip` file to a local folder.

3. Navigate to the `wperf-driver` sub-directory that contains the WindowsPerf Kernel Driver files.

4. From the command line as `Administrator`, execute the command `wperf-devgen.exe install`:

```
>wperf-devgen.exe install
```

For more details, see the [Driver Installation](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/blob/main/wperf-devgen/README.md#driver-installation) section in the `wperf-devgen.exe` documentation. Example:

5. After the driver is successfully installed, you can start using the `wperf.exe` user-space application. There's no need to use Administrator privileges for this.

6. To check your driver and user-space application compatibility, run the following command:

```shell
wperf.exe --version
```

This command will display the version of `wperf.exe`, which can help you verify if it's compatible with the installed driver. If you encounter any issues or need further assistance, feel free to ask [here](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/issues).

## Uninstalling the WindowsPerf Driver

If you no longer require the use of WindowsPerf, you have the option to uninstall the `wperf-driver` from your system. Detailed instructions for the driver uninstallation process can be found in the `Driver uninstallation` section of the [WindowsPerf documentation](https://gitlab.com/Linaro/WindowsPerf/windowsperf/-/blob/main/wperf-devgen/README.md#driver-uninstallation).

Example:

```
>wperf-devgen uninstall
```
