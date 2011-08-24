
#include "child_window_message_processor.h"

#include "base/logging.h"

#include "ui_base/view_prop.h"

namespace view
{

    static const char* const kChildWindowKey = "__CHILD_WINDOW_MESSAGE_PROCESSOR__";

    // static
    ui::ViewProp* ChildWindowMessageProcessor::Register(HWND hwnd,
        ChildWindowMessageProcessor* processor)
    {
        DCHECK(processor);
        return new ui::ViewProp(hwnd, kChildWindowKey, processor);
    }

    // static
    ChildWindowMessageProcessor* ChildWindowMessageProcessor::Get(HWND hwnd)
    {
        return reinterpret_cast<ChildWindowMessageProcessor*>(
            ui::ViewProp::GetValue(hwnd, kChildWindowKey));
    }

} //namespace view