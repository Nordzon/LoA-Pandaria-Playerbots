#include "TestHarness.h"

int main()
{
    int before = 0;
    for (auto const& c : ArpgTest::Cases())
    {
        before = ArpgTest::Failures();
        c.fn();
        std::printf("%s  %s\n", ArpgTest::Failures() == before ? "PASS" : "FAIL", c.name);
    }
    std::printf("%zu tests, %d failed checks\n", ArpgTest::Cases().size(), ArpgTest::Failures());
    return ArpgTest::Failures() == 0 ? 0 : 1;
}
