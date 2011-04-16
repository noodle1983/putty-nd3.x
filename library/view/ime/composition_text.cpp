
#include "composition_text.h"

namespace view
{

    CompositionText::CompositionText() {}

    CompositionText::~CompositionText() {}

    void CompositionText::Clear()
    {
        text.clear();
        underlines.clear();
        selection = Range();
    }

} //namespace view