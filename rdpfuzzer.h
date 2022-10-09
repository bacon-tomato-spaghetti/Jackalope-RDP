#include "fuzzer.h"
#include "sampledelivery.h"

class RDPFuzzer : public Fuzzer
{
public:
    RDPFuzzer::RDPFuzzer(const char *host, u_short port);

    Mutator *CreateMutator(int argc, char **argv, ThreadContext *tc);
    bool TrackHotOffsets() override { return true; }

    SocketSampleDelivery *CreateSampleDelivery(int argc, char **argv, ThreadContext *tc);

    const char *host;
    u_short port;
};
