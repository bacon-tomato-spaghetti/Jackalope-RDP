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
cd Jackalope
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

After `-G`, write your own cmake generator. To check this, Do `cmake -h`.

Use `Release` instead of `Debug` to remove debugging symbols.

## Usage

### Command

Execute in Command Prompt.

```powershell
fuzzer.exe -in <input directory> -out <output directory> -rdpconf <RDP config file> -nthreads <number of threads> -clean_target_on_coverage false -persist <Jackalope options> -- mstsc <mstsc options except /v>
```

For example,

```powershell
fuzzer.exe -in in -out out -rdpconf rdp.conf -nthreads 2 -instrument_module mstscax.dll -target_module mstscax.dll -clean_target_on_coverage false -persist -target_offset 0x484800 -iterations 10000 -cmp_coverage -dump_coverage -- mstsc /w:1000 /h:800
```

`0x484800` is offset of `CRdpAudioPlaybackSVCPlugin::OpenEventFn()`.

`rdp.conf` is like

```
192.168.174.133:12345
192.168.174.134:12345
```

If you want to run fuzzer for other RDP services, edit command line after `--`. And also, you should edit `RDPFuzzer::CreateRDPThreadContext()`. This function adds `/v:<server ip address>` to command line.
