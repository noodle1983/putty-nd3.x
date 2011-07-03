
#include "separator.h"

#include "ui_gfx/canvas.h"

#include "ui_base/accessibility/accessible_view_state.h"

namespace view
{

    // static
    const char Separator::kViewClassName[] = "view/Separator";

    // The separator height in pixels.
    const int kSeparatorHeight = 1;

    // Default color of the separator.
    const SkColor kDefaultColor = SkColorSetARGB(255, 233, 233, 233);

    Separator::Separator()
    {
        set_focusable(false);
    }

    Separator::~Separator() {}

    ////////////////////////////////////////////////////////////////////////////////
    // Separator, View overrides:

    gfx::Size Separator::GetPreferredSize()
    {
        return gfx::Size(width(), kSeparatorHeight);
    }

    void Separator::GetAccessibleState(ui::AccessibleViewState* state)
    {
        state->role = ui::AccessibilityTypes::ROLE_SEPARATOR;
    }

    void Separator::Paint(gfx::Canvas* canvas)
    {
        canvas->FillRectInt(kDefaultColor, x(), y(), width(), height());
    }

    std::string Separator::GetClassName() const
    {
        return kViewClassName;
    }

} //namespace view