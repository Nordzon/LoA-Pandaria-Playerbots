// Event recorder for tests (m2 task 1.1, docs/TESTING.md §3c). When Arpg.Test.RecordEvents = 1, our code notes what it did
// per character ("unique=KegBreaker removed=41150 healed=12345"); `.arpg test events <char> [clear]` reads it over SOAP.
// Off by default: Record() is a cheap no-op then.
#ifndef ARPG_TEST_EVENTS_H
#define ARPG_TEST_EVENTS_H

#include <cstdint>
#include <string>
#include <vector>

namespace Arpg::TestEvents
{
    bool Enabled();
    // key = character guid counter (or 0 for global events). Keeps the last 200 lines per key.
    void Record(uint64_t key, std::string line);
    std::vector<std::string> Get(uint64_t key, size_t maxLines);
    void Clear(uint64_t key);
}

#endif
