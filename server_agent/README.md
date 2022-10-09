# Server agents

## Introduction

Python programs which should be run in server for RDP client fuzzing.

## Implemented files

### wtsapi.py

Implementation of WTS API in Python using `ctypes`.

### server_agent.py

This program receives input(PDU) from client through socket, and sends it to client through RDP virtual channel(For example, `RDPSND`).

### autoexec.py

This program executes `server_agent.py` continuously.

## Usage

```powershell
python3 autoexec.py
```
