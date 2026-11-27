#include "Character/StartQuests.h"

#include "ArpgConfig.h"
#include "ArpgHooks.h"

#include "DatabaseEnv.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptMgr.h"

#include <map>
#include <mutex>
#include <tuple>

namespace Arpg::Character
{
    namespace
    {
        using Key = std::tuple<uint32, uint8, uint8>;   // quest, race, class
        std::map<Key, bool> g_rows;                     // -> autocomplete
        std::mutex g_lock;

        // -1 = no row, 0 = allowed, 1 = allowed and completes on accept.
        int Lookup(uint32 questId, uint8 race, uint8 cls)
        {
            std::lock_guard<std::mutex> l(g_lock);
            auto it = g_rows.find(Key{ questId, race, cls });
            return it == g_rows.end() ? -1 : (it->second ? 1 : 0);
        }

        bool QuestClassAllowed(Player const* player, uint32 questId)
        {
            if (!sArpgConfig.Get().startQuestsEnabled)
                return false;
            return Lookup(questId, player->GetRace(), player->GetClass()) >= 0;
        }

        class StartQuestsPlayerScript : public PlayerScript
        {
        public:
            StartQuestsPlayerScript() : PlayerScript("arpg_start_quests") { }

            void OnQuestAdded(Player* player, Quest const* quest) override
            {
                if (!sArpgConfig.Get().startQuestsEnabled || !quest)
                    return;
                if ((quest->GetAllowableClasses() & player->GetClassMask()) != 0)
                    return;                                             // the player's own class quest: play it normally
                if (Lookup(quest->GetQuestId(), player->GetRace(), player->GetClass()) == 1)
                    player->CompleteQuest(quest->GetQuestId());
            }
        };
    }

    std::string LoadStartQuests()
    {
        std::map<Key, bool> rows;
        if (QueryResult r = WorldDatabase.Query("SELECT quest, race, class, autocomplete FROM arpg_quest_proxy"))
            do
            {
                Field* f = r->Fetch();
                rows[Key{ f[0].GetUInt32(), f[1].GetUInt8(), f[2].GetUInt8() }] = f[3].GetUInt8() != 0;
            } while (r->NextRow());
        std::lock_guard<std::mutex> l(g_lock);
        g_rows = std::move(rows);
        return "startquests=" + std::to_string(g_rows.size());
    }

    std::string DescribeQuestProxy(uint8_t race, uint8_t cls, uint32_t questId)
    {
        int v = Lookup(questId, race, cls);
        return "allowed=" + std::to_string(v >= 0 ? 1 : 0) + " autocomplete=" + std::to_string(v == 1 ? 1 : 0)
            + " enabled=" + std::to_string(sArpgConfig.Get().startQuestsEnabled ? 1 : 0);
    }

    void InstallStartQuests()
    {
        ArpgHooks::QuestClassAllowed = &QuestClassAllowed;
        new StartQuestsPlayerScript();
    }
}
