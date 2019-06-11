libcarbon uses [CMake](https://cmake.org) as build system. CMake 3.9.6 or higher is required. For testing, [Google Test Framework](https://github.com/google/googletest) is required.  

The basic usage is 
```
cmake . &&
make  &&
make tests  &&
make test  &&
sudo make install
```
After installation, link against `libcarbon` and use
 
```#include <carbon/carbon.h>```

in your project in order to use the library. 

By default, all targets are built as release configuration. To build all targets in *debug mode* (without compiler 
optimization, with debug symbols enabled, and other debug-related features available), use 
`cmake -DBUILD_TYPE=Debug .`. The carbon library internally tracks some 
information in a log. To To turn on trace, information, warn or debug log in debug mode, set the options
`-DLOG_TRACE=on`, `-DLOG_INFO=on`, `-DLOG_WARN=on`, and `-DLOG_DEBUG=on` for `cmake`. Hence, to turn on debug mode
with all logs, use `cmake -DBUILD_TYPE=Debug -DLOG_TRACE=on -DLOG_INFO=on -DLOG_WARN=on -DLOG_DEBUG=on .`.


A tool to work with CARBON files (called `carbon-tool`) is shipped with this library.
The build process is 
```
cmake .
make carbon-tool
```
After a successful build, the tool is located in the `build` directory. The tool supports the POSIX standard for its arguments. Type `build/carbon-tool` for usage instructions.

Examples files are located in the `examples` directory, and are build with
```
cmake .
make examples-${name}
```
where `${name}`  is the topic (e.g., `error`). See source files in the `examples` 
directory to get a list of possible targets.
