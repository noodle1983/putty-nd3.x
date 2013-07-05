#ifndef PUTTY_VIEW_H
#define PUTTY_VIEW_H

#include <algorithm>
#include "view/view.h"

class NativePuttyController;

namespace view
{

    // 实现无窗口的richedit控件.
    class PuttyView : public View
	{
	public:
		static const char kViewClassName[];

        explicit PuttyView();
        virtual ~PuttyView();

		// Lay out the child Views (set their bounds based on sizing heuristics
        // specific to the current Layout Manager)


	protected:
        virtual void Layout();
		virtual void Paint(gfx::Canvas* canvas);
		NativePuttyController* puttyController_;

	};

}

#endif