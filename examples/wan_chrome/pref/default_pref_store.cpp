
#include "default_pref_store.h"

DefaultPrefStore::DefaultPrefStore() {}

DefaultPrefStore::~DefaultPrefStore() {}

void DefaultPrefStore::SetDefaultValue(const std::string& key, Value* value)
{
    CHECK(GetValue(key, NULL) == READ_NO_VALUE);
    SetValue(key, value);
}

Value::ValueType DefaultPrefStore::GetType(const std::string& key) const
{
    Value* value;
    return GetValue(key, &value)==READ_OK ? value->GetType()
        : Value::TYPE_NULL;
}