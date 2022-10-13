from socket import gethostbyname, gethostname, socket, AF_INET, SOCK_STREAM
import time
from wtsapi import *

HOST = gethostbyname(gethostname())
PORT = 12345
SIZE = 9999  # maximum buffer size

server_socket = socket(AF_INET, SOCK_STREAM)
server_socket.bind((HOST, PORT))
server_socket.listen()
print('[+] listening...')

client_socket, client_addr = server_socket.accept()
print(f'[+] Client addr: {client_addr}')

RDPConnected = False

while True:
    server_socket.settimeout(20)
    data = client_socket.recv(SIZE)
    dataSize = len(data)
    print(f'[+] {dataSize}bytes received')
    hexdump(data, len(data))

    if not RDPConnected:
        RDPServer = OpenServer(b'localhost')
        RDPConnected = True

        while True:
            RDPSND = VirtualChannelOpen(b'RDPSND', False)
            if RDPSND:
                break
            else:
                time.sleep(1)

    VirtualChannelWrite(RDPSND, data)
