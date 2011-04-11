
#ifndef __wan_chrome_pref_json_pref_store_h__
#define __wan_chrome_pref_json_pref_store_h__

#pragma once

#include "base/scoped_ptr.h"

#include "../important_file_writer.h"
#include "persistent_pref_store.h"

namespace base
{
    class FilePath;

    class MessageLoopProxy;
}

class DictionaryValue;
class Value;

// A writable PrefStore implementation that is used for user preferences.
class JsonPrefStore : public PersistentPrefStore,
    public ImportantFileWriter::DataSerializer
{
public:
    // |file_message_loop_proxy| is the MessageLoopProxy for a thread on which
    // file I/O can be done.
    JsonPrefStore(const base::FilePath& pref_filename,
        base::MessageLoopProxy* file_message_loop_proxy);
    virtual ~JsonPrefStore();

    // PrefStore overrides:
    virtual ReadResult GetValue(const std::string& key, Value** result) const;
    virtual void AddObserver(PrefStore::Observer* observer);
    virtual void RemoveObserver(PrefStore::Observer* observer);

    // PersistentPrefStore overrides:
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
    // ImportantFileWriter::DataSerializer overrides:
    virtual bool SerializeData(std::string* output);

    base::FilePath path_;

    scoped_ptr<DictionaryValue> prefs_;

    bool read_only_;

    // Helper for safely writing pref data.
    ImportantFileWriter writer_;

    ObserverList<PrefStore::Observer, true> observers_;

    DISALLOW_COPY_AND_ASSIGN(JsonPrefStore);
};

#endif //__wan_chrome_pref_json_pref_store_h__