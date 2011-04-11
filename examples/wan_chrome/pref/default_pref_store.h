
#ifndef __wan_chrome_pref_default_pref_store_h__
#define __wan_chrome_pref_default_pref_store_h__

#pragma once

#include "base/values.h"

#include "value_map_pref_store.h"

// This PrefStore keeps track of default preference values set when a
// preference is registered with the PrefService.
class DefaultPrefStore : public ValueMapPrefStore
{
public:
    DefaultPrefStore();
    virtual ~DefaultPrefStore();

    // Stores a new |value| for |key|. Assumes ownership of |value|.
    void SetDefaultValue(const std::string& key, Value* value);

    // Returns the registered type for |key| or Value::TYPE_NULL if the |key|
    // has not been registered.
    Value::ValueType GetType(const std::string& key) const;

private:
    DISALLOW_COPY_AND_ASSIGN(DefaultPrefStore);
};

#endif //__wan_chrome_pref_default_pref_store_h__