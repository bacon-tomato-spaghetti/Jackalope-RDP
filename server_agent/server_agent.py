from socket import socket, AF_INET, SOCK_STREAM
import time
from wtsapi import *

HOST = '127.0.0.2'
PORT = 12345
SIZE = 1600  # maximum buffer size

VCHandleValid = False

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

    if not VCHandleValid:
        RDPServer = OpenServer(b'localhost')
        while True:
            RDPSND = VirtualChannelOpen(b'RDPSND', False)
            if RDPSND:
                VCHandleValid = True
                break
            else:
                time.sleep(1)

    if not VirtualChannelWrite(RDPSND, data):
        VCHandleValid = False