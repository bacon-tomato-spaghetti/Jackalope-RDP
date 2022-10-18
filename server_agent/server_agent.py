from socket import socket, AF_INET, SOCK_STREAM
import time
from wtsapi import *

HOST = '127.0.0.2'
PORT = 12345
SIZE = 9999  # maximum buffer size

VCHandleValid = False  # check if virtual channel handle is valid

server_socket = socket(AF_INET, SOCK_STREAM)
server_socket.bind((HOST, PORT))
server_socket.listen()
print('[+] listening...')

client_socket, client_addr = server_socket.accept()
print(f'[+] Client addr: {client_addr}')

while True:
    data = client_socket.recv(SIZE)
    dataSize = len(data)
    print(f'[+] {dataSize}bytes received')
    hexdump(data, len(data))

    RDPServer = OpenServer(b'localhost')

    while True:
        RDPSND = VirtualChannelOpen(b'RDPSND', False)
        if RDPSND:
            VCHandleValid = True
            break
        else:
            time.sleep(1)

    for i in range(30):
        if VirtualChannelWrite(RDPSND, data):
            break
        else:
            if i == 29:
                VCHandleValid = False
            else:
                time.sleep(1)
    if not VCHandleValid:
        continue

    while True:
        data = client_socket.recv(SIZE)
        dataSize = len(data)
        print(f'[+] {dataSize}bytes received')
        hexdump(data, len(data))

        for i in range(30):
            if VirtualChannelWrite(RDPSND, data):
                break
            else:
                if i == 29:
                    VCHandleValid = False
                else:
                    time.sleep(1)
        if not VCHandleValid:
            break
