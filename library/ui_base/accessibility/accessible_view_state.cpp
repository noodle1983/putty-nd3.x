
#include "accessible_view_state.h"

namespace ui
{

    AccessibleViewState::AccessibleViewState()
        : role(AccessibilityTypes::ROLE_CLIENT),
        state(0),
        selection_start(-1),
        selection_end(-1),
        index(-1),
        count(-1) {}

    AccessibleViewState::~AccessibleViewState() {}

} //namespace ui