
#ifndef __view_accelerator_handler_h__
#define __view_accelerator_handler_h__

#pragma once

#include <set>

#include "message/message_loop.h"

namespace view
{

    // This class delegates the key messages to the associated FocusManager class
    // for the window that is receiving these messages for accelerator processing.
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