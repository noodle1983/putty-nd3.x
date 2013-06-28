#ifndef PUTTY_VIEW_H
#define PUTTY_VIEW_H

#include "view/view.h"

namespace view
{

    // 实现无窗口的richedit控件.
    class PuttyView : public View
	{
	public:
		static const char kViewClassName[];

        explicit PuttyView();
        virtual ~PuttyView();

	protected:
        virtual void Layout();

	};

}

#endif