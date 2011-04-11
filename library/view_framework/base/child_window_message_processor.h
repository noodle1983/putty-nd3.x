
#ifndef __view_framework_child_window_message_processor_h__
#define __view_framework_child_window_message_processor_h__

#pragma once

#include <windows.h>

namespace view
{

    class ViewProp;

    // Windows sends a handful of messages to the parent window rather than the
    // window itself. For example, selection changes of a rich edit (EN_SELCHANGE)
    // are sent to the parent, not the window. Typically such message are best
    // dealt with by the window rather than the parent. WidgetWin allows for
    // registering a ChildWindowMessageProcessor to handle such messages.
    class ChildWindowMessageProcessor
    {
    public:
        // Registers |processor| for |hwnd|. The caller takes ownership of the
        // returned object.
        static ViewProp* Register(HWND hwnd,
            ChildWindowMessageProcessor* processor);

        // Returns the ChildWindowMessageProcessor for |hwnd|, NULL if there isn't
        // one.
        static ChildWindowMessageProcessor* Get(HWND hwnd);

        // Invoked for any messages that are sent to the parent and originated from
        // the HWND this ChildWindowMessageProcessor was registered for. Returns true
        // if the message was handled with a valid result in |result|. Returns false
        // if the message was not handled.
        virtual bool ProcessMessage(UINT message,
            WPARAM w_param,
            LPARAM l_param,
            LRESULT* result) = 0;

    protected:
        virtual ~ChildWindowMessageProcessor() {}
    };

} //namespace view

#endif //__view_framework_child_window_message_processor_h__