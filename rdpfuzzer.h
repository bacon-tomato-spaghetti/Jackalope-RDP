#include "fuzzer.h"
#include "sampledelivery.h"

class RDPFuzzer : public Fuzzer
{
public:
    RDPFuzzer::RDPFuzzer(const char *rdpconf);

    void PrintUsage();

    class RDPThreadContext : public ThreadContext
    {
    public:
        RDPFuzzer *fuzzer;
        const char *host;
        u_short port;
    };
    RDPThreadContext *RDPFuzzer::CreateRDPThreadContext(int argc, char **argv, int thread_id, const char *host, u_short port);

    Mutator *RDPFuzzer::CreateMutator(int argc, char **argv, ThreadContext *tc) override;
    bool TrackHotOffsets() override { return true; }

    SocketSampleDelivery *CreateSampleDelivery(int argc, char **argv, RDPThreadContext *tc);

    void Run(int argc, char **argv);

    const char *rdpconf; // config file (contains host and port of sockets of RDP servers)
};
