from ctypes import windll, create_string_buffer, cast
from ctypes.wintypes import HANDLE, LPSTR, PCHAR, DWORD, ULONG, PULONG, BOOL

wtsapi = windll.wtsapi32

WTS_CURRENT_SESSION = DWORD(-1)


def hexdump(buffer, length):
    dump = ''
    for i in range(length):
        dump += hex(buffer[i])[2:].rjust(2, '0')
        if i % 0x10 == 0xf:
            if i < length - 1:
                dump += '\n'
        else:
            dump += ' '
    print(dump)


# Open RDP server
def OpenServer(pServerName: bytearray) -> HANDLE:
    WTSOpenServerA = wtsapi.WTSOpenServerA
    # HANDLE WTSOpenServerA([in] LPSTR pServerName);
    WTSOpenServerA.argtypes = [LPSTR]
    WTSOpenServerA.restype = HANDLE

    hServer = cast(WTSOpenServerA(cast(pServerName, LPSTR)), HANDLE)
    if hServer:
        print(
            f'[+] {pServerName.decode()} server opened (Handle: {hex(hServer.value)})')
        return hServer
    else:
        print(f'[-] Failed to open {pServerName.decode()} server')


# Close RDP server
def CloseServer(hServer: HANDLE):
    WTSCloseServer = wtsapi.WTSCloseServer
    # void WTSCloseServer([in] HANDLE hServer);
    WTSCloseServer.argtypes = [HANDLE]

    WTSCloseServer(hServer)
    print('[+] Server closed')


# Open virtual channel
def VirtualChannelOpen(pVirtualName: bytearray, isDynVC: bool) -> HANDLE:
    WTSVirtualChannelOpenEx = wtsapi.WTSVirtualChannelOpenEx
    # HANDLE WTSVirtualChannelOpenEx([in] DWORD SessionId, [in] LPSTR pVirtualName, [in] DWORD flags);
    WTSVirtualChannelOpenEx.argtypes = [DWORD, LPSTR, DWORD]
    WTSVirtualChannelOpenEx.restype = HANDLE

    if isDynVC:
        flags = DWORD(0x00000001)  # WTS_CHANNEL_OPTION_DYNAMIC
    else:
        flags = DWORD(0x00000000)

    hChannelHandle = cast(WTSVirtualChannelOpenEx(
        WTS_CURRENT_SESSION, cast(pVirtualName, LPSTR), flags), HANDLE)
    if hChannelHandle:
        print(f'[+] {pVirtualName.decode()} virtual channel opened (Handle: {hex(hChannelHandle.value)})')
        return hChannelHandle
    else:
        print(f'[-] Failed to open {pVirtualName.decode()} virtual channel')


# Close virtual channel
def VirtualChannelClose(hChannelHandle: HANDLE) -> BOOL:
    WTSVirtualChannelClose = wtsapi.WTSVirtualChannelClose
    # BOOL WTSVirtualChannelClose([in] HANDLE hChannelHandle);
    WTSVirtualChannelClose.argtypes = [HANDLE]
    WTSVirtualChannelClose.restype = BOOL

    result = BOOL(WTSVirtualChannelClose(hChannelHandle))
    if result:
        print('[+] Virtual channel closed')
        return result
    else:
        print('[-] Failed to close virtual channel')


# Write to virtual channel
def VirtualChannelWrite(hChannelHandle: HANDLE, Buffer: bytearray) -> BOOL:
    WTSVirtualChannelWrite = wtsapi.WTSVirtualChannelWrite
    # BOOL WTSVirtualChannelWrite([in] HANDLE hChannelHandle, [in] PCHAR Buffer, [in] ULONG Length, [out] PULONG pBytesWritten);
    WTSVirtualChannelWrite.argtypes = [HANDLE, PCHAR, ULONG, PULONG]
    WTSVirtualChannelWrite.restype = BOOL

    pBytesWritten = PULONG(ULONG(0))
    result = BOOL(WTSVirtualChannelWrite(hChannelHandle, create_string_buffer(
        Buffer), ULONG(len(Buffer)), pBytesWritten))
    if result:
        print(f'[+] {pBytesWritten.contents.value}bytes written')
        hexdump(Buffer, pBytesWritten.contents.value)
        return result
    else:
        print('[-] Failed to write to virtual channel')


# Read from virtual channel
def VirtualChannelRead(hChannelHandle: HANDLE, Timeout: int, Buffer: PCHAR, BufferSize: int) -> BOOL:
    WTSVirtualChannelRead = wtsapi.WTSVirtualChannelRead
    # BOOL WTSVirtualChannelRead([in] HANDLE hChannelHandle, [in] ULONG TimeOut, [out] PCHAR Buffer, [in] ULONG BufferSize, [out] PULONG pBytesRead);
    WTSVirtualChannelRead.argtypes = [HANDLE, ULONG, PCHAR, ULONG, PULONG]
    WTSVirtualChannelRead.restype = BOOL

    pBytesRead = PULONG(ULONG(0))
    result = BOOL(WTSVirtualChannelRead(hChannelHandle, ULONG(
        Timeout), Buffer, ULONG(BufferSize), pBytesRead))
    if result:
        print(f'[+] {pBytesRead.contents.value}bytes read')
        hexdump(Buffer[0:pBytesRead.contents.value], pBytesRead.contents.value)
        return result
    else:
        print('[-] Failed to read from virtual channel')
