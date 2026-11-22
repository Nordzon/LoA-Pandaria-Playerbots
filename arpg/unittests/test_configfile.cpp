// mod_arpg config parser (arpg.conf; .arpg reload / config set rely on it).
#include "TestHarness.h"
#include "ConfigFile.h"

using namespace Arpg::Core;

TEST(config_parses_keys_comments_quotes)
{
    ConfigFile c;
    int n = c.Parse("# comment\n\nArpg.Scale.Enabled = 1\r\nArpg.Scale.HealthMod=1.25\nArpg.Name = \"hello world\"\n[section]\n");
    CHECK_EQ(n, 3);
    CHECK_EQ(c.GetInt("Arpg.Scale.Enabled", 0), 1);
    CHECK_NEAR(c.GetFloat("Arpg.Scale.HealthMod", 0.f), 1.25, 1e-6);
    CHECK(c.GetString("Arpg.Name", "") == "hello world");
    CHECK_EQ(c.BadLines(), 0);
}

TEST(config_defaults_and_bad_values)
{
    ConfigFile c;
    c.Parse("A = abc\nB =\nnot a setting\n= 5\n");
    CHECK_EQ(c.GetInt("A", 7), 7);            // not a number -> default
    CHECK_EQ(c.GetInt("B", 3), 3);            // empty -> default
    CHECK_EQ(c.GetInt("Missing", 9), 9);
    CHECK_EQ(c.BadLines(), 2);
}

TEST(config_bools_and_later_keys_win)
{
    ConfigFile c;
    c.Parse("X = 0\nX = on\nY = false\n");
    CHECK(c.GetBool("X", false));
    CHECK(!c.GetBool("Y", true));
    c.Set("Y", "1");                          // .arpg config set
    CHECK(c.GetBool("Y", false));
}
