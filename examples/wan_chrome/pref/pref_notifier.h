
#ifndef __wan_chrome_pref_pref_notifier_h__
#define __wan_chrome_pref_pref_notifier_h__

#pragma once

#include <string>

// Delegate interface used by PrefValueStore to notify its owner about changes
// to the preference values.
// TODO(mnissler, danno): Move this declaration to pref_value_store.h once we've
// cleaned up all public uses of this interface.
class PrefNotifier
{
public:
    virtual ~PrefNotifier() {}

    // Sends out a change notification for the preference identified by
    // |pref_name|.
    virtual void OnPreferenceChanged(const std::string& pref_name) = 0;

    // Broadcasts the intialization completed notification.
    virtual void OnInitializationCompleted() = 0;
};

#endif //__wan_chrome_pref_pref_notifier_h__