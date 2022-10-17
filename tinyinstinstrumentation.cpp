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

#define _CRT_SECURE_NO_WARNINGS

#include "common.h"
#include "tinyinstinstrumentation.h"
#include "litecov.h"

#include <sstream>

void TinyInstInstrumentation::Init(int argc, char **argv)
{
    instrumentation = new LiteCov();
    instrumentation->Init(argc, argv);

    persist = GetBinaryOption("-persist", argc, argv, false);
    num_iterations = GetIntOption("-iterations", argc, argv, 1);
}

RunResult TinyInstInstrumentation::Run(int argc, char **argv, uint32_t init_timeout, uint32_t timeout)
{
    DebuggerStatus status;
    RunResult ret = OTHER_ERROR;

    if (instrumentation->IsTargetFunctionDefined())
    {
        if (cur_iteration == num_iterations)
        {
            instrumentation->Kill();
            cur_iteration = 0;
            this->ClearList();
        }
    }

    // else clear only when the target function is reached
    if (!instrumentation->IsTargetFunctionDefined())
    {
        instrumentation->ClearCoverage();
    }

    uint32_t timeout1 = timeout;
    if (instrumentation->IsTargetFunctionDefined())
    {
        timeout1 = init_timeout;
    }

    if (instrumentation->IsTargetAlive() && persist)
    {
        status = instrumentation->Continue(timeout1);
    }
    else
    {
        instrumentation->Kill();
        cur_iteration = 0;
        status = instrumentation->Run(argc, argv, timeout1);
    }

    // if target function is defined,
    // we should wait until it is hit
    if (instrumentation->IsTargetFunctionDefined())
    {
        if (status != DEBUGGER_TARGET_START)
        {
            // try again with a clean process
            WARN("Target function not reached, retrying with a clean process\n");
            instrumentation->Kill();
            cur_iteration = 0;
            status = instrumentation->Run(argc, argv, init_timeout);
        }

        if (status != DEBUGGER_TARGET_START)
        {
            switch (status)
            {
            case DEBUGGER_CRASHED:
                // FATAL("Process crashed before reaching the target method\n");
                SAY("[-] PROGRAM ABORT : Process crashed before reaching the target method\n");
                SAY("         Location : %s(), %s:%u\n\n", __FUNCTION__, __FILE__, __LINE__);
                break;
            case DEBUGGER_HANGED:
                // FATAL("Process hanged before reaching the target method\n");
                SAY("[-] PROGRAM ABORT : Process hanged before reaching the target method\n");
                SAY("         Location : %s(), %s:%u\n\n", __FUNCTION__, __FILE__, __LINE__);
                break;
            case DEBUGGER_PROCESS_EXIT:
                // FATAL("Process exited before reaching the target method\n");
                SAY("[-] PROGRAM ABORT : Process exited before reaching the target method\n");
                SAY("         Location : %s(), %s:%u\n\n", __FUNCTION__, __FILE__, __LINE__);
                break;
            default:
                // FATAL("An unknown problem occured before reaching the target method\n");
                SAY("[-] PROGRAM ABORT : An unknown problem occured before reaching the target method\n");
                SAY("         Location : %s(), %s:%u\n\n", __FUNCTION__, __FILE__, __LINE__);
                break;
            }
        }

        instrumentation->ClearCoverage();

        status = instrumentation->Continue(timeout);
    }

    switch (status)
    {
    case DEBUGGER_CRASHED:
        ret = CRASH;
        instrumentation->Kill();
        break;
    case DEBUGGER_HANGED:
        ret = HANG;
        instrumentation->Kill();
        break;
    case DEBUGGER_PROCESS_EXIT:
        ret = OK;
        if (instrumentation->IsTargetFunctionDefined())
        {
            WARN("Process exit during target function\n");
            ret = HANG;
        }
        break;
    case DEBUGGER_TARGET_END:
        if (instrumentation->IsTargetFunctionDefined())
        {
            ret = OK;
            cur_iteration++;
        }
        else
        {
            // FATAL("Unexpected status received from the debugger\n");
            SAY("[-] PROGRAM ABORT : Unexpected status received from the debugger\n");
            SAY("         Location : %s(), %s:%u\n\n", __FUNCTION__, __FILE__, __LINE__);
        }
        break;
    default:
        // FATAL("Unexpected status received from the debugger\n");
        SAY("[-] PROGRAM ABORT : Unexpected status received from the debugger\n");
        SAY("         Location : %s(), %s:%u\n\n", __FUNCTION__, __FILE__, __LINE__);
        break;
    }

    return ret;
}

RunResult TinyInstInstrumentation::RunWithCrashAnalysis(int argc, char **argv, uint32_t init_timeout, uint32_t timeout)
{
    // clean process when reproducing crashes
    instrumentation->Kill();
    // disable instrumentation when reproducing crashes
    instrumentation->DisableInstrumentation();
    RunResult ret = Run(argc, argv, init_timeout, timeout);
    instrumentation->Kill();
    instrumentation->EnableInstrumentation();
    return ret;
}

void TinyInstInstrumentation::CleanTarget()
{
    instrumentation->Kill();
}

bool TinyInstInstrumentation::HasNewCoverage()
{
    return instrumentation->HasNewCoverage();
}

void TinyInstInstrumentation::GetCoverage(Coverage &coverage, bool clear_coverage)
{
    instrumentation->GetCoverage(coverage, clear_coverage);
}

void TinyInstInstrumentation::ClearCoverage()
{
    instrumentation->ClearCoverage();
}

void TinyInstInstrumentation::IgnoreCoverage(Coverage &coverage)
{
    instrumentation->IgnoreCoverage(coverage);
}

TinyInstInstrumentation::~TinyInstInstrumentation()
{
    instrumentation->Kill();
    delete instrumentation;
}

std::string TinyInstInstrumentation::GetCrashName()
{
    LiteCov::Exception exception = instrumentation->GetLastException();
    std::stringstream stream;
    switch (exception.type)
    {
    case LiteCov::ExceptionType::ACCESS_VIOLATION:
        stream << "access_violation";
        break;
    case LiteCov::ExceptionType::ILLEGAL_INSTRUCTION:
        stream << "illegal_instruction";
        break;
    case LiteCov::ExceptionType::STACK_OVERFLOW:
        stream << "stack_overflow";
        break;
    default:
        stream << "other";
        break;
    }
    stream << "_";
    stream << AnonymizeAddress(exception.ip);
    stream << "_";
    stream << AnonymizeAddress(exception.access_address);
    return stream.str();
}

void TinyInstInstrumentation::AppendToList(std::string sample)
{
    this->cur_loop_inputs.push_back(sample);
}
std::vector<std::string> TinyInstInstrumentation::ExportList()
{
    return this->cur_loop_inputs;
}
void TinyInstInstrumentation::ClearList()
{
    this->cur_loop_inputs.clear();
}

uint64_t TinyInstInstrumentation::GetReturnValue()
{
    return instrumentation->GetTargetReturnValue();
}
