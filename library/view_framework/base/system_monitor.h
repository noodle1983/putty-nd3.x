
#ifndef __view_framework_system_monitor_h__
#define __view_framework_system_monitor_h__

#pragma once

#include "message_framework/observer_list_threadsafe.h"
#include "message_framework/timer.h"

// Class for monitoring various system-related subsystems
// such as power management, network status, etc.
// TODO(mbelshe):  Add support beyond just power management.
class SystemMonitor
{
public:
    // Normalized list of power events.
    enum PowerEvent
    {
        POWER_STATE_EVENT,  // The Power status of the system has changed.
        SUSPEND_EVENT,      // The system is being suspended.
        RESUME_EVENT        // The system is being resumed.
    };

    // Create SystemMonitor. Only one SystemMonitor instance per application
    // is allowed.
    SystemMonitor();
    ~SystemMonitor();

    // Get the application-wide SystemMonitor (if not present, returns NULL).
    static SystemMonitor* Get();

    // Power-related APIs

    // Is the computer currently on battery power.
    // Can be called on any thread.
    bool BatteryPower() const
    {
        // Using a lock here is not necessary for just a bool.
        return battery_in_use_;
    }

    // Callbacks will be called on the thread which creates the SystemMonitor.
    // During the callback, Add/RemoveObserver will block until the callbacks
    // are finished. Observers should implement quick callback functions; if
    // lengthy operations are needed, the observer should take care to invoke
    // the operation on an appropriate thread.
    class PowerObserver
    {
    public:
        // Notification of a change in power status of the computer, such
        // as from switching between battery and A/C power.
        virtual void OnPowerStateChange(bool on_battery_power) {}

        // Notification that the system is suspending.
        virtual void OnSuspend() {}

        // Notification that the system is resuming.
        virtual void OnResume() {}

    protected:
        virtual ~PowerObserver() {}
    };

    // Add a new observer.
    // Can be called from any thread.
    // Must not be called from within a notification callback.
    void AddObserver(PowerObserver* obs);

    // Remove an existing observer.
    // Can be called from any thread.
    // Must not be called from within a notification callback.
    void RemoveObserver(PowerObserver* obs);

    // Windows-specific handling of a WM_POWERBROADCAST message.
    // Embedders of this API should hook their top-level window
    // message loop and forward WM_POWERBROADCAST through this call.
    void ProcessWmPowerBroadcastMessage(int event_id);

    // Cross-platform handling of a power event.
    void ProcessPowerMessage(PowerEvent event_id);

private:
    // Platform-specific method to check whether the system is currently
    // running on battery power.  Returns true if running on batteries,
    // false otherwise.
    bool IsBatteryPower();

    // Checks the battery status and notifies observers if the battery
    // status has changed.
    void BatteryCheck();

    // Functions to trigger notifications.
    void NotifyPowerStateChange();
    void NotifySuspend();
    void NotifyResume();

    scoped_refptr<ObserverListThreadSafe<PowerObserver> > observer_list_;
    bool battery_in_use_;
    bool suspended_;

    base::OneShotTimer<SystemMonitor> delayed_battery_check_;

    DISALLOW_COPY_AND_ASSIGN(SystemMonitor);
};

#endif //__view_framework_system_monitor_h__