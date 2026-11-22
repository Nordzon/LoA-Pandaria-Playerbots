#include "ConfigFile.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace
{
    std::string Trim(std::string const& s)
    {
        size_t b = s.find_first_not_of(" \t\r\n");
        if (b == std::string::npos)
            return "";
        size_t e = s.find_last_not_of(" \t\r\n");
        return s.substr(b, e - b + 1);
    }
}

namespace Arpg::Core
{
    int ConfigFile::Parse(std::string const& text)
    {
        std::istringstream in(text);
        std::string line;
        int count = 0;
        while (std::getline(in, line))
        {
            std::string t = Trim(line);
            if (t.empty() || t[0] == '#' || t[0] == '[')
                continue;
            size_t eq = t.find('=');
            if (eq == std::string::npos || eq == 0)
            {
                ++_badLines;
                continue;
            }
            std::string key = Trim(t.substr(0, eq));
            std::string value = Trim(t.substr(eq + 1));
            if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
                value = value.substr(1, value.size() - 2);
            if (key.empty())
            {
                ++_badLines;
                continue;
            }
            _values[key] = value;
            ++count;
        }
        return count;
    }

    bool ConfigFile::LoadFile(std::string const& path, std::string& error)
    {
        std::ifstream f(path, std::ios::binary);
        if (!f)
        {
            error = "cannot open " + path;
            return false;
        }
        std::ostringstream ss;
        ss << f.rdbuf();
        Parse(ss.str());
        return true;
    }

    std::string ConfigFile::GetString(std::string const& key, std::string const& def) const
    {
        auto it = _values.find(key);
        return it == _values.end() ? def : it->second;
    }

    int ConfigFile::GetInt(std::string const& key, int def) const
    {
        auto it = _values.find(key);
        if (it == _values.end() || it->second.empty())
            return def;
        char* end = nullptr;
        long v = std::strtol(it->second.c_str(), &end, 10);
        return (end && *end == '\0') ? int(v) : def;
    }

    float ConfigFile::GetFloat(std::string const& key, float def) const
    {
        auto it = _values.find(key);
        if (it == _values.end() || it->second.empty())
            return def;
        char* end = nullptr;
        float v = std::strtof(it->second.c_str(), &end);
        return (end && *end == '\0') ? v : def;
    }

    bool ConfigFile::GetBool(std::string const& key, bool def) const
    {
        auto it = _values.find(key);
        if (it == _values.end())
            return def;
        std::string const& v = it->second;
        if (v == "1" || v == "true" || v == "yes" || v == "on")
            return true;
        if (v == "0" || v == "false" || v == "no" || v == "off")
            return false;
        return def;
    }
}
