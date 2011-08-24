
#include "repeat_controller.h"

namespace view
{

    // The delay before the first and then subsequent repeats. Values taken from
    // XUL code: http://mxr.mozilla.org/seamonkey/source/layout/xul/base/src/nsRepeatService.cpp#52
    static const int kInitialRepeatDelay = 250;
    static const int kRepeatDelay = 50;

    ///////////////////////////////////////////////////////////////////////////////
    // RepeatController, public:

    RepeatController::RepeatController(RepeatCallback* callback)
        : callback_(callback) {}

    RepeatController::~RepeatController() {}

    void RepeatController::Start()
    {
        // The first timer is slightly longer than subsequent repeats.
        timer_.Start(base::TimeDelta::FromMilliseconds(kInitialRepeatDelay), this,
            &RepeatController::Run);
    }

    void RepeatController::Stop()
    {
        timer_.Stop();
    }

    ///////////////////////////////////////////////////////////////////////////////
    // RepeatController, private:

    void RepeatController::Run()
    {
        timer_.Start(base::TimeDelta::FromMilliseconds(kRepeatDelay), this,
            &RepeatController::Run);
        callback_->Run();
    }

} //namespace view