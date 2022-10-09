/*
Copyright 2020 Google LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

// modification for RDP fuzzing
#include <WinSock2.h>
#include <WS2tcpip.h>

#include "common.h"
#include "sampledelivery.h"

int FileSampleDelivery::DeliverSample(Sample *sample)
{
    return sample->Save(filename.c_str());
}

SHMSampleDelivery::SHMSampleDelivery(char *name, size_t size)
{
    shmobj.Open(name, size);
    shm = shmobj.GetData();
}

SHMSampleDelivery::~SHMSampleDelivery()
{
    shmobj.Close();
}

int SHMSampleDelivery::DeliverSample(Sample *sample)
{
    uint32_t *size_ptr = (uint32_t *)shm;
    unsigned char *data_ptr = shm + 4;
    *size_ptr = (uint32_t)sample->size;
    memcpy(data_ptr, sample->bytes, sample->size);
    return 1;
}


// modification for RDP fuzzing

SocketSampleDelivery::SocketSampleDelivery(const char *host, u_short port)
{
    int WSAStartup_result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (WSAStartup_result)
    {
        puts("[-] Failed to initialize socket");
        exit(-1);
    }

    ZeroMemory(&this->sockAddr, sizeof(this->sockAddr));
    this->sockAddr.sin_family = AF_INET;
    inet_pton(AF_INET, host, &this->sockAddr.sin_addr.S_un.S_addr);
    this->sockAddr.sin_port = htons(port);

    this->sock = socket(AF_INET, SOCK_STREAM, 0);
    if (this->sock == INVALID_SOCKET)
    {
        puts("[-] Failed to create socket");
        exit(-1);
    }

    int sockfd = connect(this->sock, (sockaddr *)&this->sockAddr, sizeof(this->sockAddr));
    if (sockfd == SOCKET_ERROR)
    {
        puts("[-] Failed to connect to server");
        exit(-1);
    }
}

SocketSampleDelivery::~SocketSampleDelivery()
{
    closesocket(this->sock);
    WSACleanup();
}

int SocketSampleDelivery::DeliverSample(Sample *sample)
{
    return (send(this->sock, sample->bytes, (int)sample->size, 0) != 0);
}
