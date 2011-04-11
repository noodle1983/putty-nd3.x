
#include "system_monitor.h"

#include "base/logging.h"

#include "message_framework/message_loop.h"

static SystemMonitor* g_system_monitor = NULL;

// The amount of time (in ms) to wait before running the initial
// battery check.
static int kDelayedBatteryCheckMs = 10 * 1000;

SystemMonitor::SystemMonitor()
: observer_list_(new ObserverListThreadSafe<PowerObserver>()),
battery_in_use_(false),
suspended_(false)
{
    DCHECK(!g_system_monitor);
    g_system_monitor = this;

    DCHECK(MessageLoop::current());
    delayed_battery_check_.Start(
        base::TimeDelta::FromMilliseconds(kDelayedBatteryCheckMs),
        this, &SystemMonitor::BatteryCheck);
}

SystemMonitor::~SystemMonitor()
{
    DCHECK_EQ(this, g_system_monitor);
    g_system_monitor = NULL;
}

// static
SystemMonitor* SystemMonitor::Get()
{
    return g_system_monitor;
}

void SystemMonitor::ProcessPowerMessage(PowerEvent event_id)
{
    // Suppress duplicate notifications.  Some platforms may
    // send multiple notifications of the same event.
    switch(event_id)
    {
    case POWER_STATE_EVENT:
        {
            bool on_battery = IsBatteryPower();
            if(on_battery != battery_in_use_)
            {
                battery_in_use_ = on_battery;
                NotifyPowerStateChange();
            }
        }
        break;
    case RESUME_EVENT:
        if(suspended_)
        {
            suspended_ = false;
            NotifyResume();
        }
        break;
    case SUSPEND_EVENT:
        if(!suspended_)
        {
            suspended_ = true;
            NotifySuspend();
        }
        break;
    }
}

void SystemMonitor::AddObserver(PowerObserver* obs)
{
    observer_list_->AddObserver(obs);
}

void SystemMonitor::RemoveObserver(PowerObserver* obs)
{
    observer_list_->RemoveObserver(obs);
}

void SystemMonitor::ProcessWmPowerBroadcastMessage(int event_id)
{
    PowerEvent power_event;
    switch(event_id)
    {
    case PBT_APMPOWERSTATUSCHANGE:  // The power status changed.
        power_event = POWER_STATE_EVENT;
        break;
    case PBT_APMRESUMEAUTOMATIC:  // Resume from suspend.
        //case PBT_APMRESUMESUSPEND:  // User-initiated resume from suspend.
        // We don't notify for this latter event
        // because if it occurs it is always sent as a
        // second event after PBT_APMRESUMEAUTOMATIC.
        power_event = RESUME_EVENT;
        break;
    case PBT_APMSUSPEND:  // System has been suspended.
        power_event = SUSPEND_EVENT;
        break;
    default:
        return;

        // Other Power Events:
        // PBT_APMBATTERYLOW - removed in Vista.
        // PBT_APMOEMEVENT - removed in Vista.
        // PBT_APMQUERYSUSPEND - removed in Vista.
        // PBT_APMQUERYSUSPENDFAILED - removed in Vista.
        // PBT_APMRESUMECRITICAL - removed in Vista.
        // PBT_POWERSETTINGCHANGE - user changed the power settings.
    }
    ProcessPowerMessage(power_event);
}

// Function to query the system to see if it is currently running on
// battery power.  Returns true if running on battery.
bool SystemMonitor::IsBatteryPower()
{
    SYSTEM_POWER_STATUS status;
    if(!GetSystemPowerStatus(&status))
    {
        LOG(ERROR) << "GetSystemPowerStatus failed: " << GetLastError();
        return false;
    }
    return (status.ACLineStatus == 0);
}

void SystemMonitor::NotifyPowerStateChange()
{
    VLOG(1) << L"PowerStateChange: " << (BatteryPower() ? L"On" : L"Off")
        << L" battery";
    observer_list_->Notify(&PowerObserver::OnPowerStateChange, BatteryPower());
}

void SystemMonitor::NotifySuspend()
{
    VLOG(1) << L"Power Suspending";
    observer_list_->Notify(&PowerObserver::OnSuspend);
}

void SystemMonitor::NotifyResume()
{
    VLOG(1) << L"Power Resuming";
    observer_list_->Notify(&PowerObserver::OnResume);
}

void SystemMonitor::BatteryCheck()
{
    ProcessPowerMessage(SystemMonitor::POWER_STATE_EVENT);
}