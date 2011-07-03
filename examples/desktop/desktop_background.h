
#ifndef __desktop_background_h__
#define __desktop_background_h__

#pragma once

#include "view/background.h"

namespace view
{
    namespace desktop
    {

        class DesktopBackground : public Background
        {
        public:
            DesktopBackground();
            virtual ~DesktopBackground();

        private:
            // Overridden from Background:
            virtual void Paint(gfx::Canvas* canvas, View* view) const;

            DISALLOW_COPY_AND_ASSIGN(DesktopBackground);
        };

    } //namespace desktop
} //namespace view

#endif //__desktop_background_h__