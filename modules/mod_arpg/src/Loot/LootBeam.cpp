#include "Loot/LootBeam.h"

#include "ArpgConfig.h"
#include "Scaling/WorldScaling.h"
#include "Test/TestEvents.h"

#include "Creature.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "TemporarySummon.h"
#include "Timer.h"

#include <mutex>
#include <unordered_map>
#include <vector>

namespace
{
    uint32 const MarkerEntry = 15384;      // "OLDWorld Trigger": invisible, not selectable
    uint32 const InvisibleDisplay = 11686;
    uint32 const BeamLifeMs = 5 * MINUTE * IN_MILLISECONDS;

    struct Beam { uint64 corpse; uint64 marker; uint32 madeAt; };
    std::mutex g_lock;
    std::unordered_map<uint32, std::vector<Beam>> g_beams;   // player guid -> beams of its kills
    std::unordered_map<uint32, uint32> g_lastCheck;

    bool WorthPicking(Creature* corpse, uint32 minQuality)
    {
        for (LootItem const& li : corpse->loot.items)
        {
            if (li.is_looted || li.currency)
                continue;
            if (ItemTemplate const* t = sObjectMgr->GetItemTemplate(li.itemid))
                if (t->Quality >= minQuality)
                    return true;
        }
        return false;
    }

    void RemoveMarker(Player* p, uint64 marker)
    {
        if (Creature* m = ObjectAccessor::GetCreature(*p, ObjectGuid(marker)))
            m->DespawnOrUnsummon();
    }

    class ArpgLootBeamPlayerScript : public PlayerScript
    {
    public:
        ArpgLootBeamPlayerScript() : PlayerScript("arpg_loot_beam") { }

        // Once a second: drop the beams of corpses that were looted, despawned or are too old.
        void OnUpdate(Player* p, uint32 /*diff*/) override
        {
            if (!p->IsInWorld() || !Arpg::Scaling::IsRealPlayer(p))
                return;
            uint32 key = p->GetGUID().GetCounter();
            uint32 now = getMSTime();
            std::vector<Beam> beams;
            {
                std::lock_guard<std::mutex> l(g_lock);
                uint32& last = g_lastCheck[key];
                if (last && getMSTimeDiff(last, now) < 1000)
                    return;
                last = now;
                auto it = g_beams.find(key);
                if (it == g_beams.end() || it->second.empty())
                    return;
                beams = it->second;
            }
            uint32 minQuality = sArpgConfig.Get().lootBeamMinQuality;
            std::vector<Beam> keep;
            for (Beam const& b : beams)
            {
                Creature* corpse = ObjectAccessor::GetCreature(*p, ObjectGuid(b.corpse));
                bool gone = !corpse || !corpse->IsInWorld() || corpse->IsAlive() || corpse->loot.isLooted() ||
                    !WorthPicking(corpse, minQuality) || getMSTimeDiff(b.madeAt, now) > BeamLifeMs;
                if (gone)
                    RemoveMarker(p, b.marker);
                else
                    keep.push_back(b);
            }
            std::lock_guard<std::mutex> l(g_lock);
            g_beams[key] = keep;
        }

        void OnLogout(Player* p) override
        {
            std::vector<Beam> beams;
            {
                std::lock_guard<std::mutex> l(g_lock);
                beams = g_beams[p->GetGUID().GetCounter()];
                g_beams.erase(p->GetGUID().GetCounter());
                g_lastCheck.erase(p->GetGUID().GetCounter());
            }
            for (Beam const& b : beams)
                RemoveMarker(p, b.marker);
        }
    };
}

namespace Arpg::Loot
{
    bool MaybeBeamCorpse(Player* owner, Creature* corpse)
    {
        Arpg::Settings const& s = sArpgConfig.Get();
        if (!owner || !corpse || !s.lootBeamAura || !Arpg::Scaling::IsRealPlayer(owner))
            return false;
        if (!WorthPicking(corpse, s.lootBeamMinQuality))
            return false;
        TempSummon* m = corpse->SummonCreature(MarkerEntry, corpse->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, Milliseconds(BeamLifeMs));
        if (!m)
            return false;
        m->SetDisplayId(InvisibleDisplay);
        m->SetReactState(REACT_PASSIVE);
        if (Aura* a = m->AddAura(s.lootBeamAura, m))
        {
            a->SetMaxDuration(int32(BeamLifeMs));
            a->SetDuration(int32(BeamLifeMs));
        }
        {
            std::lock_guard<std::mutex> l(g_lock);
            g_beams[owner->GetGUID().GetCounter()].push_back({ corpse->GetGUID().GetRawValue(), m->GetGUID().GetRawValue(), getMSTime() });
        }
        Arpg::TestEvents::Record(owner->GetGUID().GetCounter(), "loot beam corpse=" + std::to_string(corpse->GetEntry()));
        return true;
    }

    int ActiveBeams(Player* owner)
    {
        std::lock_guard<std::mutex> l(g_lock);
        auto it = g_beams.find(owner->GetGUID().GetCounter());
        return it == g_beams.end() ? 0 : int(it->second.size());
    }

    void InstallLootBeam()
    {
        new ArpgLootBeamPlayerScript();
    }
}
