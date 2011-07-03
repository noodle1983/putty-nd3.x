
#include "scroll_bar.h"

#include "ui_base/accessibility/accessible_view_state.h"

namespace view
{

    /////////////////////////////////////////////////////////////////////////////
    //
    // ScrollBar implementation
    //
    /////////////////////////////////////////////////////////////////////////////

    ScrollBar::ScrollBar(bool is_horiz) : is_horiz_(is_horiz),
        controller_(NULL), max_pos_(0) {}

    ScrollBar::~ScrollBar() {}

    void ScrollBar::GetAccessibleState(ui::AccessibleViewState* state)
    {
        state->role = ui::AccessibilityTypes::ROLE_SCROLLBAR;
    }

    bool ScrollBar::IsHorizontal() const
    {
        return is_horiz_;
    }

    void ScrollBar::SetController(ScrollBarController* controller)
    {
        controller_ = controller;
    }

    ScrollBarController* ScrollBar::GetController() const
    {
        return controller_;
    }

    void ScrollBar::Update(int viewport_size, int content_size, int current_pos)
    {
        max_pos_ = std::max(0, content_size - viewport_size);
    }

    int ScrollBar::GetMaxPosition() const
    {
        return max_pos_;
    }

    int ScrollBar::GetMinPosition() const
    {
        return 0;
    }

} //namespace view