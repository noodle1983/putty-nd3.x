
#include "child_window_message_processor.h"

#include "base/logging.h"

#include "../base/view_prop.h"

namespace view
{

    static const char* const kChildWindowKey = "__CHILD_WINDOW_MESSAGE_PROCESSOR__";

    // static
    ViewProp* ChildWindowMessageProcessor::Register(
        HWND hwnd,
        ChildWindowMessageProcessor* processor)
    {
        DCHECK(processor);
        return new ViewProp(hwnd, kChildWindowKey, processor);
    }

    // static
    ChildWindowMessageProcessor* ChildWindowMessageProcessor::Get(HWND hwnd)
    {
        return reinterpret_cast<ChildWindowMessageProcessor*>(
            ViewProp::GetValue(hwnd, kChildWindowKey));
    }

} //namespace view