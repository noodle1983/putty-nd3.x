
#ifndef __ui_base_composition_text_h__
#define __ui_base_composition_text_h__

#pragma once

#include "base/string16.h"

#include "composition_underline.h"
#include "ui_base/range/range.h"

namespace ui
{

    // A struct represents the status of an ongoing composition text.
    struct CompositionText
    {
        CompositionText();
        ~CompositionText();

        void Clear();

        // Content of the composition text.
        string16 text;

        // Underline information of the composition text.
        // They must be sorted in ascending order by their start_offset and cannot be
        // overlapped with each other.
        CompositionUnderlines underlines;

        // Selection range in the composition text. It represents the caret position
        // if the range length is zero. Usually it's used for representing the target
        // clause (on Windows). Gtk doesn't have such concept, so background color is
        // usually used instead.
        Range selection;
    };

} //namespace ui

#endif //__ui_base_composition_text_h__