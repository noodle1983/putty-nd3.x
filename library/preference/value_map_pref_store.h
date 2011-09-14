
#ifndef __value_map_pref_store_h__
#define __value_map_pref_store_h__

#pragma once

#include "base/basic_types.h"
#include "base/observer_list.h"

#include "pref_store.h"
#include "pref_value_map.h"

// A basic PrefStore implementation that uses a simple name-value map for
// storing the preference values.
class ValueMapPrefStore : public PrefStore
{
public:
    typedef std::map<std::string, base::Value*>::iterator iterator;
    typedef std::map<std::string, base::Value*>::const_iterator const_iterator;

    ValueMapPrefStore();
    virtual ~ValueMapPrefStore();

    // PrefStore overrides:
    virtual ReadResult GetValue(const std::string& key,
        const base::Value** value) const;
    virtual void AddObserver(PrefStore::Observer* observer);
    virtual void RemoveObserver(PrefStore::Observer* observer);

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;

protected:
    // Store a |value| for |key| in the store. Also generates an notification if
    // the value changed. Assumes ownership of |value|, which must be non-NULL.
    void SetValue(const std::string& key, base::Value* value);

    // Remove the value for |key| from the store. Sends a notification if there
    // was a value to be removed.
    void RemoveValue(const std::string& key);

    // Notify observers about the initialization completed event.
    void NotifyInitializationCompleted();

private:
    PrefValueMap prefs_;

    ObserverList<PrefStore::Observer, true> observers_;

    DISALLOW_COPY_AND_ASSIGN(ValueMapPrefStore);
};

#endif //__value_map_pref_store_h__