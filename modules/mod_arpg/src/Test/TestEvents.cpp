#include "Test/TestEvents.h"

#include "ArpgConfig.h"

#include <deque>
#include <mutex>
#include <unordered_map>

namespace
{
    std::mutex g_lock;
    std::unordered_map<uint64_t, std::deque<std::string>> g_events;
    size_t const MaxLinesPerKey = 200;
}

namespace Arpg::TestEvents
{
    bool Enabled()
    {
        return sArpgConfig.Get().recordEvents;
    }

    void Record(uint64_t key, std::string line)
    {
        if (!Enabled())
            return;
        std::lock_guard<std::mutex> l(g_lock);
        std::deque<std::string>& q = g_events[key];
        q.push_back(std::move(line));
        while (q.size() > MaxLinesPerKey)
            q.pop_front();
    }

    std::vector<std::string> Get(uint64_t key, size_t maxLines)
    {
        std::lock_guard<std::mutex> l(g_lock);
        std::vector<std::string> out;
        auto it = g_events.find(key);
        if (it == g_events.end())
            return out;
        size_t start = it->second.size() > maxLines ? it->second.size() - maxLines : 0;
        for (size_t i = start; i < it->second.size(); ++i)
            out.push_back(it->second[i]);
        return out;
    }

    void Clear(uint64_t key)
    {
        std::lock_guard<std::mutex> l(g_lock);
        g_events.erase(key);
    }
}
