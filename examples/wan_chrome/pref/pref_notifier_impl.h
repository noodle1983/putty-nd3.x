
#ifndef __wan_chrome_pref_pref_notifier_impl_h__
#define __wan_chrome_pref_pref_notifier_impl_h__

#pragma once

#include <hash_map>

#include "base/threading/non_thread_safe.h"

#include "message_framework/observer_list.h"

#include "pref_notifier.h"

class PrefService;
class NotificationObserver;

// The PrefNotifier implementation used by the PrefService.
class PrefNotifierImpl : public PrefNotifier, public NonThreadSafe
{
public:
    explicit PrefNotifierImpl(PrefService* pref_service);
    virtual ~PrefNotifierImpl();

    // If the pref at the given path changes, we call the observer's Observe
    // method with PREF_CHANGED.
    void AddPrefObserver(const char* path, NotificationObserver* obs);
    void RemovePrefObserver(const char* path, NotificationObserver* obs);

    // PrefNotifier overrides.
    virtual void OnPreferenceChanged(const std::string& pref_name);
    virtual void OnInitializationCompleted();

protected:
    // A map from pref names to a list of observers. Observers get fired in the
    // order they are added. These should only be accessed externally for unit
    // testing.
    typedef ObserverList<NotificationObserver> NotificationObserverList;
    typedef stdext::hash_map<std::string, NotificationObserverList*>
        PrefObserverMap;

    const PrefObserverMap* pref_observers() const { return &pref_observers_; }

private:
    // For the given pref_name, fire any observer of the pref. Virtual so it can
    // be mocked for unit testing.
    virtual void FireObservers(const std::string& path);

    // Weak reference; the notifier is owned by the PrefService.
    PrefService* pref_service_;

    PrefObserverMap pref_observers_;

    DISALLOW_COPY_AND_ASSIGN(PrefNotifierImpl);
};

#endif //__wan_chrome_pref_pref_notifier_impl_h__