Installation
============

Pre-requests
------------
* 64-bits POSIX platform, Mac OS X 10.7+ and Linux with kernel version 2.8 or higher
* [GCC](gcc.gnu.org) with C++ support, version 4.6 or higher, or [Clang](clang.llvm.org) version 3.0 or higher
* [libstdc++](http://gcc.gnu.org/libstdc++/), the version comes with GCC should work, or libc++ comes with Mac OS X
* [Boost](www.boost.org), version 1.50.0 or higher
* [CMake](www.cmake.org), version 2.8 or higher
* [ICU](icu-project.org), version 4.8 or higher
* [Log4cplus](log4cplus.sf.net), version 1.0.4.1
* [ProtoBuf](https://developers.google.com/protocol-buffers), version 2.4 or higher

Installation
------------
1. Make sure all pre-requests are installed and working properly

    1. Mac OS X

        1. Install Xcode from [AppStore](https://itunes.apple.com/us/app/xcode/id497799835)

        2. Install Xcode command line tools, which can be downloaded within Xcode app, or from [Apple Developer Connection site](https://developer.apple.com/downloads/)

        3. Install [MacPorts](www.macports.org/install.php)

        4. Install Boost, ICU, Log4CPlus, and ProtoBuf from MacPorts

            `sudo port install boost cmake icu log4cplus protobuf-cpp`

        5. Build 'Argos.xcodeproj' with Xcode

        [Homebrew](https://github.com/mxcl/homebrew) may work but not tested.

    2. Ubuntu/Debian
    
        Recent releases(since Oct 2012) have new version of dependent packages and should work out-of-the-box.

    3. CentOS/RHEL
        
        These distributions ship very old versions of Boost and Log4CPlus, which are not tested.
    
        You need to download and install up-to-date versions by yourself.
    
    4. On FreeBSD, you'll need Clang instead of the aged GCC 4.2, therefor you need to compile and install all dependencies by hand.
    
        Ports cannot be used as they are still using GCC.
    
        FreeBSD migrated default compiler to Clang in 9-CURRENT recently, but the port status are unclear.

2. Run `cmake -DCMAKE_BUILD_TYPE=[Release|Debug] PATH/to/Argos` in build directory

3. Run `make` in build directory
