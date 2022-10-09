# Jackalope_RDP

## Introduction

We customized Jackalope developed by Google Project Zero for RDP client fuzzing.

Original repository: https://github.com/googleprojectzero/Jackalope

## Installation

### Environment

Windows 10 x64

### Download

```powershell
git clone https://github.com/bacon-tomato-spaghetti/Jackalope_RDP.git
cd Jackalope_RDP
git clone --recurse-submodules https://github.com/googleprojectzero/TinyInst.git
```

### Build

Build in Developer PowerShell for VS.

```powershell
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Debug
```

After `-G`, write your own cmake generator. To check this, do `cmake -h`.

Use `Release` instead of `Debug` to remove debugging symbols.

## Usage

### Command

Execute in Command Prompt.

```powershell
fuzzer.exe -in <input directory> -out <output directory> -rdpconf <RDP config file> -channel <virtual channel to run fuzzing on> -nthreads <number of threads> -clean_target_on_coverage false -persist <Jackalope options> -- mstsc <mstsc options except /v>
```

For example,

```powershell
fuzzer.exe -in in -out out -rdpconf rdp.conf -channel RDPSND -nthreads 2 -instrument_module mstscax.dll -target_module mstscax.dll -clean_target_on_coverage false -persist -target_offset 0x484800 -iterations 10000 -cmp_coverage -dump_coverage -- mstsc /w:1000 /h:800
```

- Options
  - `-in`: Input directory name. There are seed files which contain sample PDUs.
  - `-out`: Output directory name. Jackalope automatically create and set this directory.
  - `-rdpconf`: RDP config file name.
  - `-channel`: Virtual channel name to fuzz.
  - `-nthreads`: Number of fuzzing threads. This value had better be equal to number of lines of RDP config file.
  - Jackalope options: https://github.com/googleprojectzero/Jackalope#running-jackalope
  - TinyInst options: https://github.com/googleprojectzero/TinyInst#command-line-options

If you want to run fuzzer for other RDP services, edit command line after `--`. And also, you should edit `RDPFuzzer::CreateRDPThreadContext()`, which adds `/v:<server ip address>` to command line.

### RDP config file

```
<IP address of server1>:<socket port of server1>
<IP address of server2>:<socket port of server2>
...
```

For example,

```
192.168.174.133:12345
192.168.174.134:12345
```



## OutputFilter

With writing `RDPFuzzer::OutputFilter()`, You can slightly edit mutated PDU. For example, you can remove invalid value of `msgType` or `bodySize` field.
