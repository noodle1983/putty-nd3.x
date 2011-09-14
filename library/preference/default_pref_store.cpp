
#include "default_pref_store.h"

DefaultPrefStore::DefaultPrefStore() {}

DefaultPrefStore::~DefaultPrefStore() {}

void DefaultPrefStore::SetDefaultValue(const std::string& key,
                                       base::Value* value)
{
    CHECK(GetValue(key, NULL) == READ_NO_VALUE);
    SetValue(key, value);
}

base::Value::Type DefaultPrefStore::GetType(const std::string& key) const
{
    const base::Value* value;
    return GetValue(key, &value) == READ_OK ?
        value->GetType() : base::Value::TYPE_NULL;
}