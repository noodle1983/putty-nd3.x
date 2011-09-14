
#include "value_map_pref_store.h"

#include <algorithm>

#include "base/stl_utilinl.h"
#include "base/value.h"

ValueMapPrefStore::ValueMapPrefStore() {}

ValueMapPrefStore::~ValueMapPrefStore() {}

PrefStore::ReadResult ValueMapPrefStore::GetValue(const std::string& key,
                                                  const base::Value** value) const
{
    return prefs_.GetValue(key, value) ? READ_OK : READ_NO_VALUE;
}

void ValueMapPrefStore::AddObserver(PrefStore::Observer* observer)
{
    observers_.AddObserver(observer);
}

void ValueMapPrefStore::RemoveObserver(PrefStore::Observer* observer)
{
    observers_.RemoveObserver(observer);
}

void ValueMapPrefStore::SetValue(const std::string& key, base::Value* value)
{
    if(prefs_.SetValue(key, value))
    {
        FOR_EACH_OBSERVER(Observer, observers_, OnPrefValueChanged(key));
    }
}

void ValueMapPrefStore::RemoveValue(const std::string& key)
{
    if(prefs_.RemoveValue(key))
    {
        FOR_EACH_OBSERVER(Observer, observers_, OnPrefValueChanged(key));
    }
}

void ValueMapPrefStore::NotifyInitializationCompleted()
{
    FOR_EACH_OBSERVER(Observer, observers_, OnInitializationCompleted(true));
}

ValueMapPrefStore::iterator ValueMapPrefStore::begin()
{
    return prefs_.begin();
}

ValueMapPrefStore::iterator ValueMapPrefStore::end()
{
    return prefs_.end();
}

ValueMapPrefStore::const_iterator ValueMapPrefStore::begin() const
{
    return prefs_.begin();
}

ValueMapPrefStore::const_iterator ValueMapPrefStore::end() const
{
    return prefs_.end();
}