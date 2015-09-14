#ifndef __HYPER_BASE_CONTROLPLANE_HH__
#define __HYPER_BASE_CONTROLPLANE_HH__

#include <string.h>
#include <algorithm>
#include <utility>
#include <vector>

#include "hyper/base/pardg5v.hh"

class StatsTable
{
  public:

    typedef uint64_t StatsType;
    typedef void (*stats_update_t)(StatsType *stats, void *args);

  protected:

    int max_metrics;
    std::vector<StatsType *> stats;

    stats_update_t updator;
    void *updator_args;

  public:

    StatsTable(int max_metrics, int max_entries,
               stats_update_t updator = StatsTable::default_updator,
               void *args = NULL)
      : max_metrics(max_metrics), stats(max_entries),
        updator(updator), updator_args(args)
    {
        for (int i=0; i<max_entries; i++)
            stats[i] = new StatsType[max_metrics];
        resetStats();
    }
    ~StatsTable()
    {
        std::for_each (stats.begin(), stats.end(),
                       [](StatsType *s) { delete[] s; });
    }

  public:

    template <typename T>
    T* getStatsPtr(LDomID ldomID)
    {
        //assert(ldomID < stats.size());
        if (ldomID >= stats.size()) {
            warn("StatsTable::getStatsPtr() receive invalid ldomID, assume #0.\n");
            ldomID = 0;
        }
        return (T*)stats[ldomID];
    }

    void updateStatsPtr(LDomID ldomID, StatsType *data)
    {
        assert(ldomID < stats.size());
        memcpy(stats[ldomID], data, sizeof(StatsType) * max_metrics);
    }

    StatsType getStats(LDomID ldomID, int metric)
    {
        assert(ldomID < stats.size());
        assert(metric < max_metrics);
        return stats[ldomID][metric];
    }

    void update(LDomID ldomID)
    {
        assert(ldomID < stats.size());
        (*updator)(stats[ldomID], updator_args);
    }

    void resetStats()
    {
        std::for_each (stats.begin(), stats.end(), [this](StatsType *s) {
            memset(s, 0, sizeof(StatsType) * max_metrics);
        });
    }

  protected:

    static void default_updator(StatsType *stats, void *args) {}

};

struct TriggerEntry {
    LDomID ldom;
    uint16_t metric;
    union {
        uint16_t data;
        struct {
            unsigned short type : 4;
            unsigned short mode : 1;
            unsigned short __res1 : 3;
            unsigned short delay : 4;
            unsigned short __res2 : 4;
       } param;
    } comparator;
    uint64_t value;
    uint16_t notify_id;
    uint16_t triggers;
    bool reported;
};

#define COMPARATOR_EQ	0
#define COMPARATOR_LT	1
#define COMPARATOR_GT	2

class TriggerTable
{
  protected:

    StatsTable &stats;
    std::vector<struct TriggerEntry> entries;

  public:

    TriggerTable(StatsTable &stats, int max_entries = 128)
      : stats(stats), entries(max_entries)
    {
        for_each(entries.begin(), entries.end(), [](TriggerEntry &entry) {
            memset(&entry, 0, sizeof(TriggerEntry));
            entry.metric = -1;
        });

        // TODO: Test ONLY
        TriggerEntry &entry = entries[0];
        entry.ldom = 1;
        entry.metric = 0;
        entry.comparator.data = 0;
        entry.comparator.param.type = COMPARATOR_EQ;
        entry.value = 2000;
        entry.notify_id = 0x88;
    }

  public:

    typedef std::vector<std::pair<uint16_t /*notify_id*/, uint16_t /*trigger_id*/> >
            TriggeredList;
    TriggeredList checkTrigger(LDomID ldomID)
    {
        TriggeredList tlist;
        std::for_each(entries.begin(), entries.end(), [&](TriggerEntry &entry) {
            if (entry.metric == (uint16_t)-1)
                return;
            if (entry.ldom != ldomID)
                return;
            uint64_t stats_value = stats.getStats(ldomID, entry.metric);
            bool is_trigger = false;
            switch (entry.comparator.param.type) {
              case COMPARATOR_LT:
                is_trigger = stats_value < entry.value;
                break;
              case COMPARATOR_GT:
                is_trigger = stats_value > entry.value;
                break;
              case COMPARATOR_EQ:
                is_trigger = stats_value == entry.value;
                break;
            }
            if (is_trigger) {
                entry.triggers ++;
                if (!entry.comparator.param.mode ||
                    1<<entry.comparator.param.delay == entry.triggers)
                {
                    tlist.push_back(std::make_pair(entry.notify_id, (uint16_t)0));
                    return;
                }
            }
        });
        return tlist;
    }

};

#endif
