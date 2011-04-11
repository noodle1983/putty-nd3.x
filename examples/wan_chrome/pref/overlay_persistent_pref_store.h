
#ifndef __wan_chrome_pref_overlay_persistent_pref_store_h__
#define __wan_chrome_pref_overlay_persistent_pref_store_h__

#pragma once

#include "message_framework/observer_list.h"

#include "pref_value_map.h"
#include "persistent_pref_store.h"

// PersistentPrefStore that directs all write operations into a in-memory
// PrefValueMap. Read operations are first answered by the PrefValueMap.
// If the PrefValueMap does not contain a value for the requested key,
// the look-up is passed on to an underlying PersistentPrefStore |underlay_|.
class OverlayPersistentPrefStore : public PersistentPrefStore,
    public PrefStore::Observer
{
public:
    explicit OverlayPersistentPrefStore(PersistentPrefStore* underlay);
    virtual ~OverlayPersistentPrefStore();

    // Returns true if a value has been set for the |key| in this
    // OverlayPersistentPrefStore, i.e. if it potentially overrides a value
    // from the |underlay_|.
    virtual bool IsSetInOverlay(const std::string& key) const;

    // Methods of PrefStore.
    virtual void AddObserver(PrefStore::Observer* observer);
    virtual void RemoveObserver(PrefStore::Observer* observer);
    virtual bool IsInitializationComplete() const;
    virtual ReadResult GetValue(const std::string& key, Value** result) const;

    // Methods of PersistentPrefStore.
    virtual void SetValue(const std::string& key, Value* value);
    virtual void SetValueSilently(const std::string& key, Value* value);
    virtual void RemoveValue(const std::string& key);
    virtual bool ReadOnly() const;
    virtual PrefReadError ReadPrefs();
    virtual bool WritePrefs();
    virtual void ScheduleWritePrefs();
    // TODO(battre) remove this function
    virtual void ReportValueChanged(const std::string& key);

private:
    // Methods of PrefStore::Observer.
    virtual void OnPrefValueChanged(const std::string& key);
    virtual void OnInitializationCompleted();

    ObserverList<PrefStore::Observer, true> observers_;
    PrefValueMap overlay_;
    scoped_refptr<PersistentPrefStore> underlay_;

    DISALLOW_COPY_AND_ASSIGN(OverlayPersistentPrefStore);
};

#endif //__wan_chrome_pref_overlay_persistent_pref_store_h__