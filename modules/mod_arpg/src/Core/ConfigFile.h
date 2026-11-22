// Pure: parses "Key = Value" config text (arpg.conf). No game headers - unit-tested in arpg/unittests.
#ifndef ARPG_CORE_CONFIGFILE_H
#define ARPG_CORE_CONFIGFILE_H

#include <map>
#include <string>

namespace Arpg::Core
{
    class ConfigFile
    {
    public:
        // Lines: "Key = Value", "# comment", blank. Values may be quoted ("..."). Later keys win.
        // Returns the number of keys read; malformed lines are counted in BadLines().
        int Parse(std::string const& text);
        bool LoadFile(std::string const& path, std::string& error);

        bool Has(std::string const& key) const { return _values.count(key) != 0; }
        void Set(std::string const& key, std::string const& value) { _values[key] = value; }
        std::string GetString(std::string const& key, std::string const& def) const;
        int GetInt(std::string const& key, int def) const;
        float GetFloat(std::string const& key, float def) const;
        bool GetBool(std::string const& key, bool def) const;

        int BadLines() const { return _badLines; }
        std::map<std::string, std::string> const& Values() const { return _values; }

    private:
        std::map<std::string, std::string> _values;
        int _badLines = 0;
    };
}

#endif
