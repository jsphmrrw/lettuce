# Lettuce Parser/Interpreter

## Building on Windows

A Windows `build.bat` script is provided for your convenience, which will create a `build` folder, and try to call `cl.exe` (the MSVC compiler) directly to compile the program. This script assumes that `cl.exe` is installed and is known about. To get this to happen, you'll need to call the `vcvarsall.bat` script provided with Microsoft Visual Studio (yes, I wish it was just a standalone executable too).

The script expects to be called inside of the project folder.

## Building on Linux

A Linux `build.sh` script is provided as well, which you should just be able to call. I didn't write this on Linux and give the file proper permissions, so you might need to do `sudo chmod a+x build.sh`. It assumes it has access to `gcc` (which it should).

The script expects to be called inside of the project folder.
