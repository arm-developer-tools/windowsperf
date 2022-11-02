# Build wperf-driver

You can build `wperf-driver` project from command line:

```
> devenv windowsperf.sln /Rebuild "Debug|ARM64" /Project wperf-driver\wperf-driver.vcxproj
```

# Kernel Driver Installation
Kernel driver can be installed on ARM64 machine with below command:

```
> devcon install windows-driver.inf Root\WPERFDRIVER
```

Other uses of `devcon` command are:

```
> devcon remove windows-driver.inf Root\WPERFDRIVER
```

And

```
> devcon status windows-driver.inf Root\WPERFDRIVER
```

## Device Console (devcon.exe)

For more information about `devcon` command please visit [Device Console Commands](https://learn.microsoft.com/en-us/windows-hardware/drivers/devtest/devcon-general-commands).

Please note that `devcon.exe` should be located in `c:\Program Files (x86)\Windows Kits\10\Tools\ARM64\` on your ARM64 machine.
See some details [here](https://learn.microsoft.com/en-us/windows-hardware/drivers/gettingstarted/writing-a-kmdf-driverbased-on-a-template#install-the-driver).

If you face issue installing driver you might look at the last sections of `c:\windows\inf\setupapi.app.log` and `c:\windows\inf\setupapi.dev.log`. They might provide some hints.

## Extra step (installing non-signed driver)

To enable installing non-signed driver on WOA ARM64, one needs to execute:

```
> bcdedit /set testsigning on
```

using Windows Admin account (via CMD). Note: This change takes affect after reboot.
This might further require you to disable secure boot from BIOS if it has been enabled.
