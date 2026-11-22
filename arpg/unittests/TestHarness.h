// Minimal test harness: TEST(name) { CHECK(cond); CHECK_EQ(a, b); }. Exit code = failed checks.
#ifndef ARPG_TEST_HARNESS_H
#define ARPG_TEST_HARNESS_H

#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace ArpgTest
{
    struct Case { const char* name; std::function<void()> fn; };
    inline std::vector<Case>& Cases() { static std::vector<Case> c; return c; }
    inline int& Failures() { static int f = 0; return f; }
    struct Register { Register(const char* n, std::function<void()> f) { Cases().push_back({ n, std::move(f) }); } };
}

#define ARPG_CAT2(a, b) a##b
#define ARPG_CAT(a, b) ARPG_CAT2(a, b)
#define TEST(name) \
    static void name(); \
    static ArpgTest::Register ARPG_CAT(reg_, name)(#name, name); \
    static void name()

#define CHECK(cond) do { if (!(cond)) { ++ArpgTest::Failures(); \
    std::printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)

#define CHECK_EQ(a, b) do { auto _va = (a); auto _vb = (b); if (!(_va == _vb)) { ++ArpgTest::Failures(); \
    std::printf("  FAIL %s:%d  %s == %s  (%s vs %s)\n", __FILE__, __LINE__, #a, #b, \
        std::to_string(_va).c_str(), std::to_string(_vb).c_str()); } } while (0)

#define CHECK_NEAR(a, b, eps) do { double _va = (a), _vb = (b); if (_va - _vb > (eps) || _vb - _va > (eps)) { \
    ++ArpgTest::Failures(); std::printf("  FAIL %s:%d  %s ~ %s  (%f vs %f)\n", __FILE__, __LINE__, #a, #b, _va, _vb); } } while (0)

#endif
