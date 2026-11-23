#include "LootFilter.h"

#include <algorithm>
#include <sstream>

namespace Arpg::Core
{
    bool RuleMatches(FilterRule const& r, ItemFacts const& f)
    {
        if (r.minQuality >= 0 && f.quality < r.minQuality) return false;
        if (r.maxQuality >= 0 && f.quality > r.maxQuality) return false;
        if (r.usable >= 0 && f.usable != (r.usable == 1)) return false;
        if (r.itemClass >= 0 && f.itemClass != r.itemClass) return false;
        if (r.itemSubClass >= 0 && f.itemSubClass != r.itemSubClass) return false;
        if (r.minAffixes >= 0 && f.affixCount < r.minAffixes) return false;
        if (r.maxAffixes >= 0 && f.affixCount > r.maxAffixes) return false;
        if (r.minBestTier >= 0 && f.bestTier < r.minBestTier) return false;
        if (r.vsEquipped >= 0)
        {
            if (f.equippedItemLevel < 0)
                return false;   // nothing to compare with
            bool lower = f.itemLevel < f.equippedItemLevel;
            if (lower != (r.vsEquipped == 0))
                return false;
        }
        if (!r.itemIds.empty() && std::find(r.itemIds.begin(), r.itemIds.end(), f.itemId) == r.itemIds.end())
            return false;
        return true;
    }

    FilterAction EvaluateFilter(std::vector<FilterRule> const& rules, ItemFacts const& f, int& matchedRule)
    {
        matchedRule = -1;
        if (f.quest || f.unique)
            return FilterAction::Pickup;
        for (size_t i = 0; i < rules.size(); ++i)
            if (RuleMatches(rules[i], f))
            {
                matchedRule = int(i);
                return rules[i].action;
            }
        return FilterAction::Pickup;
    }

    namespace
    {
        char const* const ActionNames[] = { "pickup", "leave", "sell", "highlight" };
    }

    std::string RuleToText(FilterRule const& r)
    {
        std::ostringstream o;
        auto put = [&](char const* k, int v) { if (v >= 0) o << k << v << ' '; };
        put("quality>=", r.minQuality);
        put("quality<=", r.maxQuality);
        put("usable=", r.usable);
        put("class=", r.itemClass);
        put("sub=", r.itemSubClass);
        put("affixes>=", r.minAffixes);
        put("affixes<=", r.maxAffixes);
        put("tier>=", r.minBestTier);
        put("vs=", r.vsEquipped);
        if (!r.itemIds.empty())
        {
            o << "ids=";
            for (size_t i = 0; i < r.itemIds.size(); ++i)
                o << (i ? "," : "") << r.itemIds[i];
            o << ' ';
        }
        o << "-> " << ActionNames[int(r.action)];
        return o.str();
    }

    bool RuleFromText(std::string const& text, FilterRule& out)
    {
        FilterRule r;
        std::istringstream in(text);
        std::string tok;
        bool action = false;
        while (in >> tok)
        {
            if (tok == "->")
            {
                std::string a;
                if (!(in >> a))
                    return false;
                bool found = false;
                for (int i = 0; i < 4; ++i)
                    if (a == ActionNames[i])
                    {
                        r.action = FilterAction(i);
                        found = true;
                    }
                if (!found)
                    return false;
                action = true;
                continue;
            }
            size_t pos = tok.find_first_of("<>=");
            if (pos == std::string::npos)
                return false;
            std::string key = tok.substr(0, pos);
            std::string op;
            size_t vpos = pos;
            while (vpos < tok.size() && (tok[vpos] == '<' || tok[vpos] == '>' || tok[vpos] == '='))
                op += tok[vpos++];
            std::string val = tok.substr(vpos);
            if (val.empty())
                return false;
            if (key == "ids")
            {
                std::istringstream ids(val);
                std::string id;
                while (std::getline(ids, id, ','))
                    if (!id.empty())
                        r.itemIds.push_back(uint32_t(std::stoul(id)));
                continue;
            }
            int v = 0;
            try { v = std::stoi(val); } catch (...) { return false; }
            if (key == "quality" && op == ">=") r.minQuality = v;
            else if (key == "quality" && op == "<=") r.maxQuality = v;
            else if (key == "quality" && op == "=") r.minQuality = r.maxQuality = v;
            else if (key == "usable" && op == "=") r.usable = v ? 1 : 0;
            else if (key == "class" && op == "=") r.itemClass = v;
            else if (key == "sub" && op == "=") r.itemSubClass = v;
            else if (key == "affixes" && op == ">=") r.minAffixes = v;
            else if (key == "affixes" && op == "<=") r.maxAffixes = v;
            else if (key == "tier" && op == ">=") r.minBestTier = v;
            else if (key == "vs" && op == "=") r.vsEquipped = v ? 1 : 0;
            else return false;
        }
        if (!action)
            return false;
        out = r;
        return true;
    }
}
