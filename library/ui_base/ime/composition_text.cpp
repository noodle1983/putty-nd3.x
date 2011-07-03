
#include "composition_text.h"

namespace ui
{

    CompositionText::CompositionText() {}

    CompositionText::~CompositionText() {}

    void CompositionText::Clear()
    {
        text.clear();
        underlines.clear();
        selection = Range();
    }

} //namespace ui