
#ifndef __view_repeat_controller_h__
#define __view_repeat_controller_h__

#pragma once

#include "base/callback.h"
#include "base/scoped_ptr.h"

#include "message/timer.h"

namespace view
{

    ///////////////////////////////////////////////////////////////////////////////
    //
    // RepeatController
    //
    //  An object that handles auto-repeating UI actions. There is a longer initial
    //  delay after which point repeats become constant. Users provide a callback
    //  that is notified when each repeat occurs so that they can perform the
    //  associated action.
    //
    ///////////////////////////////////////////////////////////////////////////////
    class RepeatController
    {
    public:
        typedef Callback0::Type RepeatCallback;

        // The RepeatController takes ownership of this callback object.
        explicit RepeatController(RepeatCallback* callback);
        virtual ~RepeatController();

        // Start repeating.
        void Start();

        // Stop repeating.
        void Stop();

    private:
        RepeatController();

        // Called when the timer expires.
        void Run();

        // The current timer.
        base::OneShotTimer<RepeatController> timer_;

        scoped_ptr<RepeatCallback> callback_;

        DISALLOW_COPY_AND_ASSIGN(RepeatController);
    };

} //namespace view

#endif //__view_repeat_controller_h__