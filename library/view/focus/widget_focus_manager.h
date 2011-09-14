
#ifndef __view_widget_focus_manager_h__
#define __view_widget_focus_manager_h__

#pragma once

#include "base/basic_types.h"
#include "base/observer_list.h"

template<typename T> struct DefaultSingletonTraits;

namespace view
{

    // This interface should be implemented by classes that want to be notified when
    // the native focus is about to change.  Listeners implementing this interface
    // will be invoked for all native focus changes across the entire Chrome
    // application.  FocusChangeListeners are only called for changes within the
    // children of a single top-level native-view.
    class WidgetFocusChangeListener
    {
    public:
        virtual void NativeFocusWillChange(HWND focused_before,
            HWND focused_now) = 0;

    protected:
        virtual ~WidgetFocusChangeListener() {}
    };

    class WidgetFocusManager
    {
    public:
        // Returns the singleton instance.
        static WidgetFocusManager* GetInstance();

        // Adds/removes a WidgetFocusChangeListener |listener| to the set of
        // active listeners.
        void AddFocusChangeListener(WidgetFocusChangeListener* listener);
        void RemoveFocusChangeListener(WidgetFocusChangeListener* listener);

        // To be called when native-focus shifts from |focused_before| to
        // |focused_now|.
        // TODO(port) : Invocations to this routine are only implemented for
        // the Win32 platform.  Calls need to be placed appropriately for
        // non-Windows environments.
        void OnWidgetFocusEvent(HWND focused_before, HWND focused_now);

        // Enable/Disable notification of registered listeners during calls
        // to OnWidgetFocusEvent.  Used to prevent unwanted focus changes from
        // propagating notifications.
        void EnableNotifications() { enabled_ = true; }
        void DisableNotifications() { enabled_ = false; }

    private:
        friend struct DefaultSingletonTraits<WidgetFocusManager>;

        WidgetFocusManager();
        ~WidgetFocusManager();

        ObserverList<WidgetFocusChangeListener> focus_change_listeners_;

        bool enabled_;

        DISALLOW_COPY_AND_ASSIGN(WidgetFocusManager);
    };

    // A basic helper class that is used to disable native focus change
    // notifications within a scope.
    class AutoNativeNotificationDisabler
    {
    public:
        AutoNativeNotificationDisabler();
        ~AutoNativeNotificationDisabler();

    private:
        DISALLOW_COPY_AND_ASSIGN(AutoNativeNotificationDisabler);
    };

} //namespace view

#endif //__view_widget_focus_manager_h__