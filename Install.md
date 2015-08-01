<a href='Hidden comment: 
#summary How to install.
'></a>

You can either download the binary or compile from sources.

# Download Windows Binary #
Go to Downloads and download the pre-compiled binary.

# Compile #
## Prerequisites ##
  * libboost-system-dev 1.47 or higher.
  * g++ 4.6 or higher.

## Compiling ##
```
git clone https://code.google.com/p/udp-obfuscator/
cd udp-obfuscator
cp config.cmake.example config.cmake
```
Edit config.cmake if you need.
```
cmake .
make
```