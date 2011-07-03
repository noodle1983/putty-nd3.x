
#ifndef __view_accelerator_handler_h__
#define __view_accelerator_handler_h__

#pragma once

#include <set>

#include "base/message_loop.h"

namespace view
{

    ////////////////////////////////////////////////////////////////////////////////
    // AcceleratorHandler class
    //
    //  An object that pre-screens all UI messages for potential accelerators.
    //  Registered accelerators are processed regardless of focus within a given
    //  Widget or Window.
    //
    //  This processing is done at the Dispatcher level rather than on the Widget
    //  because of the global nature of this processing, and the fact that not all
    //  controls within a window need to be Widgets - some are native controls from
    //  the underlying toolkit wrapped by NativeViewHost.
    class AcceleratorHandler : public MessageLoopForUI::Dispatcher
    {
    public:
        AcceleratorHandler();
        // Dispatcher method. This returns true if an accelerator was processed by the
        // focus manager
        virtual bool Dispatch(const MSG& msg);

    private:
        // The keys currently pressed and consumed by the FocusManager.
        std::set<WPARAM> pressed_keys_;

        DISALLOW_COPY_AND_ASSIGN(AcceleratorHandler);
    };

} //namespace view

#endif //__view_accelerator_handler_h__