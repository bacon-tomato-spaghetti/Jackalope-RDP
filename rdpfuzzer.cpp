#include "rdpfuzzer.h"
#include "common.h"
#include "mutator.h"

// Same with BinaryFuzzer::CreateMutator() in main.cpp
Mutator *RDPFuzzer::CreateMutator(int argc, char **argv, ThreadContext *tc)
{
    bool use_deterministic_mutations = true;
    if (GetBinaryOption("-server", argc, argv, false))
    {
        // don't do deterministic mutation if a server is specified
        use_deterministic_mutations = false;
    }
    use_deterministic_mutations = GetBinaryOption("-deterministic_mutations", argc, argv, use_deterministic_mutations);

    bool deterministic_only = GetBinaryOption("-deterministic_only", argc, argv, false);

    int nrounds = GetIntOption("-iterations_per_round", argc, argv, 1000);

    char *dictionary = GetOption("-dict", argc, argv);

    // a pretty simple mutation strategy

    PSelectMutator *pselect = new PSelectMutator();

    // select one of the mutators below with corresponding
    // probablilities
    pselect->AddMutator(new ByteFlipMutator(), 0.8);
    pselect->AddMutator(new ArithmeticMutator(), 0.2);
    pselect->AddMutator(new AppendMutator(1, 128), 0.2);
    pselect->AddMutator(new BlockInsertMutator(1, 128), 0.1);
    pselect->AddMutator(new BlockFlipMutator(2, 16), 0.1);
    pselect->AddMutator(new BlockFlipMutator(16, 64), 0.1);
    pselect->AddMutator(new BlockFlipMutator(1, 64, true), 0.1);
    pselect->AddMutator(new BlockDuplicateMutator(1, 128, 1, 8), 0.1);

    InterestingValueMutator *iv_mutator = new InterestingValueMutator(true);
    if (dictionary)
        iv_mutator->AddDictionary(dictionary);
    pselect->AddMutator(iv_mutator, 0.1);

    // SpliceMutator is not compatible with -keep_samples_in_memory=0
    // as it requires other samples in memory besides the one being
    // fuzzed.
    if (GetBinaryOption("-keep_samples_in_memory", argc, argv, true))
    {
        pselect->AddMutator(new SpliceMutator(1, 0.5), 0.1);
        pselect->AddMutator(new SpliceMutator(2, 0.5), 0.1);
    }

    Mutator *pselect_or_range = pselect;

    // if we are tracking ranges, insert a RangeMutator
    // between RepeatMutator and individual mutators
    if (GetBinaryOption("-track_ranges", argc, argv, false))
    {
        RangeMutator *range_mutator = new RangeMutator(pselect);
        pselect_or_range = range_mutator;
    }

    // potentially repeat the mutation
    // (do two or more mutations in a single cycle
    // 0 indicates that actual mutation rate will be adapted
    RepeatMutator *repeater = new RepeatMutator(pselect_or_range, 0);

    if (!use_deterministic_mutations && !deterministic_only)
    {
        // and have nrounds of this per sample cycle
        NRoundMutator *mutator = new NRoundMutator(repeater, nrounds);
        return mutator;
    }
    else
    {
        MutatorSequence *deterministic_sequence = new MutatorSequence(false, true);
        // do deterministic byte flip mutations (around hot bits)
        deterministic_sequence->AddMutator(new DeterministicByteFlipMutator());
        // ..followed by deterministc interesting values
        deterministic_sequence->AddMutator(new DeterministicInterestingValueMutator(true));

        size_t deterministic_rounds, nondeterministic_rounds;
        if (deterministic_only)
        {
            deterministic_rounds = nrounds;
        }
        else
        {
            deterministic_rounds = nrounds / 2;
        }
        nondeterministic_rounds = nrounds - deterministic_rounds;

        // do 1000 rounds of derministic mutations, will switch to nondeterministic mutations
        // once deterministic mutator is "done"
        DtermininsticNondeterministicMutator *mutator =
            new DtermininsticNondeterministicMutator(
                deterministic_sequence,
                deterministic_rounds,
                repeater,
                nondeterministic_rounds);

        return mutator;
    }
}

RDPFuzzer::RDPFuzzer(const char *host, u_short port)
{
    this->host = host;
    this->port = port;
}

SocketSampleDelivery *RDPFuzzer::CreateSampleDelivery(int argc, char **argv, ThreadContext *tc)
{
    return new SocketSampleDelivery(this->host, this->port);
}
