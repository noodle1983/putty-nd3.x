
#include "widget_focus_manager.h"

#include "base/memory/singleton.h"

namespace view
{

    // WidgetFocusManager ----------------------------------------------------------

    // static
    WidgetFocusManager* WidgetFocusManager::GetInstance()
    {
        return Singleton<WidgetFocusManager>::get();
    }

    void WidgetFocusManager::AddFocusChangeListener(
        WidgetFocusChangeListener* listener)
    {
        focus_change_listeners_.AddObserver(listener);
    }

    void WidgetFocusManager::RemoveFocusChangeListener(
        WidgetFocusChangeListener* listener)
    {
        focus_change_listeners_.RemoveObserver(listener);
    }

    void WidgetFocusManager::OnWidgetFocusEvent(HWND focused_before,
        HWND focused_now)
    {
        if(enabled_)
        {
            FOR_EACH_OBSERVER(WidgetFocusChangeListener, focus_change_listeners_,
                NativeFocusWillChange(focused_before, focused_now));
        }
    }

    WidgetFocusManager::WidgetFocusManager() : enabled_(true) {}

    WidgetFocusManager::~WidgetFocusManager() {}

    // AutoNativeNotificationDisabler ----------------------------------------------

    AutoNativeNotificationDisabler::AutoNativeNotificationDisabler()
    {
        WidgetFocusManager::GetInstance()->DisableNotifications();
    }

    AutoNativeNotificationDisabler::~AutoNativeNotificationDisabler()
    {
        WidgetFocusManager::GetInstance()->EnableNotifications();
    }

} //namespace view