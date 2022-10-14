from socket import gethostbyname, gethostname, socket, AF_INET, SOCK_STREAM
import time
import datetime
from wtsapi import *

HOST = '127.0.0.2'
PORT = 12345
SIZE = 9999  # maximum buffer size
sampleDir = '.\\samples'

def saveSample(data):
    sampleFile = sampleDir + '\\sample_' + datetime.datetime.now().strftime('%Y-%m-%d-%H-%M-%S-%f')
    f = open(sampleFile, 'wb')
    f.write(data)
    f.close()

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

    saveSample(data)

    RDPServer = OpenServer(b'localhost')
    RDPConnected = True

    while True:
        RDPSND = VirtualChannelOpen(b'RDPSND', False)
        if RDPSND:
            break
        else:
            time.sleep(1)

    if not VirtualChannelWrite(RDPSND, data):
        continue

    while True:
        data = client_socket.recv(SIZE)
        dataSize = len(data)
        print(f'[+] {dataSize}bytes received')
        hexdump(data, len(data))

        saveSample(data)

        if not VirtualChannelWrite(RDPSND, data):
            break