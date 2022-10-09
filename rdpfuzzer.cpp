#include "rdpfuzzer.h"
#include "common.h"
#include "mutator.h"
#include "server.h"
#include "directory.h"
#include "thread.h"

RDPFuzzer::RDPFuzzer(const char *rdpconf)
{
    this->rdpconf = rdpconf;
}

void RDPFuzzer::PrintUsage()
{
    puts("[-] Usage: fuzzer.exe -in <input directory> -out <output directory> -rdpconf <RDP config file> -nthreads <number of threads> -clean_target_on_coverage false -persist <Jackalope options> -- mstsc <mstsc options except /v>");
    puts("[-] Example: fuzzer.exe -in in -out out -rdpconf rdp.conf -nthreads 2 -instrument_module mstscax.dll -target_module mstscax.dll -clean_target_on_coverage false -persist -target_offset 0x484800 -iterations 10000 -cmp_coverage -dump_coverage -- mstsc /w:1000 /h:800");

    exit(0);
}

RDPFuzzer::RDPThreadContext *RDPFuzzer::CreateRDPThreadContext(int argc, char **argv, int thread_id, const char *host, u_short port)
{
    RDPThreadContext *tc = new RDPThreadContext();

    tc->host = host;
    tc->port = port;

    tc->target_argc = target_argc + 1;
    tc->target_argv = (char **)calloc(tc->target_argc + 1, sizeof(char *));
    for (int i = 0; i < tc->target_argc - 1; i++)
    {
        tc->target_argv[i] = target_argv[i];
    }
    tc->target_argv[tc->target_argc - 1] = (char *)calloc(strlen(tc->host) + 4, sizeof(char));
    tc->target_argv[tc->target_argc - 1][0] = '/';
    tc->target_argv[tc->target_argc - 1][1] = 'v';
    tc->target_argv[tc->target_argc - 1][2] = ':';
    strcpy(&tc->target_argv[tc->target_argc - 1][3], tc->host);

    tc->thread_id = thread_id;
    tc->fuzzer = this;
    tc->prng = CreatePRNG(argc, argv, tc);
    tc->mutator = CreateMutator(argc, argv, tc);
    tc->instrumentation = CreateInstrumentation(argc, argv, tc);
    tc->sampleDelivery = CreateSampleDelivery(argc, argv, tc);
    tc->minimizer = CreateMinimizer(argc, argv, tc);
    tc->range_tracker = CreateRangeTracker(argc, argv, tc);
    tc->coverage_initialized = false;

    return tc;
}

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

SocketSampleDelivery *RDPFuzzer::CreateSampleDelivery(int argc, char **argv, RDPThreadContext *tc)
{
    return new SocketSampleDelivery(tc->host, tc->port);
}

void *StartRDPFuzzThread(void *arg)
{
    RDPFuzzer::RDPThreadContext *tc = (RDPFuzzer::RDPThreadContext *)arg;
    tc->fuzzer->RunFuzzerThread(tc);
    return NULL;
}

void RDPFuzzer::Run(int argc, char **argv)
{
    if (GetOption("-start_server", argc, argv))
    {
        // run the server
        printf("Running as server\n");
        CoverageServer server;
        server.Init(argc, argv);
        server.RunServer();
        return;
    }

    // printf("Fuzzer version 1.00\n");
    puts(":: Jackalope_RDP ::");

    samples_pending = 0;

    num_crashes = 0;
    num_unique_crashes = 0;
    num_hangs = 0;
    num_samples = 0;
    num_samples_discarded = 0;
    total_execs = 0;

    ParseOptions(argc, argv);

    SetupDirectories();

    if (should_restore_state)
    {
        state = RESTORE_NEEDED;
    }
    else
    {
        GetFilesInDirectory(in_dir, input_files);

        if (input_files.size() == 0)
        {
            WARN("Input directory is empty\n");
        }
        else
        {
            SAY("%d input files read\n", (int)input_files.size());
        }
        state = INPUT_SAMPLE_PROCESSING;
    }

    last_save_time = GetCurTime();


    // modification for RDP fuzzing

    const char *channel = GetOption("-channel", argc, argv);
    if (channel == NULL)
    {
        PrintUsage();
    }
    else
    {
        this->channel = channel;
    }

    FILE *fp = NULL;
    fopen_s(&fp, this->rdpconf, "r");
    for (int thread_id = 1; thread_id <= num_threads; thread_id++)
    {
        char conf[0x20] = {0};
        fgets(conf, 0x20, fp);
        int j = 0;
        for (j = 0; j < 0x20; j++)
        {
            if (conf[j] == ':')
            {
                conf[j] = '\0';
                break;
            }
        }
        const char *host = conf;
        u_short port = (u_short)atoi(&conf[j + 1]);

        RDPThreadContext *tc = CreateRDPThreadContext(argc, argv, thread_id, host, port);
        CreateThread(StartRDPFuzzThread, tc);
    }
    fclose(fp);
    

    uint64_t last_execs = 0;

    uint32_t secs_to_sleep = 1;

    while (1)
    {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
        Sleep(secs_to_sleep * 1000);
#else
        usleep(secs_to_sleep * 1000000);
#endif

        size_t num_offsets = 0;
        coverage_mutex.Lock();
        for (auto iter = fuzzer_coverage.begin(); iter != fuzzer_coverage.end(); iter++)
        {
            num_offsets += iter->offsets.size();
        }
        coverage_mutex.Unlock();

        printf("\nTotal execs: %lld\nUnique samples: %lld (%lld discarded)\nCrashes: %lld (%lld unique)\nHangs: %lld\nOffsets: %zu\nExecs/s: %lld\n", total_execs, num_samples, num_samples_discarded, num_crashes, num_unique_crashes, num_hangs, num_offsets, (total_execs - last_execs) / secs_to_sleep);
        last_execs = total_execs;

        if (state == FUZZING && dry_run)
        {
            printf("\nDry run done\n");
            exit(0);
        }
    }
}

bool RDPFuzzer::OutputFilter(Sample *original_sample, Sample *output_sample, ThreadContext *tc)
{
    *output_sample = *original_sample;

    return true;
}
