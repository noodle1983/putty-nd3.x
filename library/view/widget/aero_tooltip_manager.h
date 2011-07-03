
#ifndef __view_aero_tooltip_manager_h__
#define __view_aero_tooltip_manager_h__

#pragma once

#include "base/memory/ref_counted.h"

#include "tooltip_manager_win.h"

namespace view
{

    ///////////////////////////////////////////////////////////////////////////////
    // AeroTooltipManager
    //
    //  Default Windows tooltips are broken when using our custom window frame
    //  - as soon as the tooltip receives a WM_MOUSEMOVE event, it starts spewing
    //  NCHITTEST messages at its parent window (us). These messages have random
    //  x/y coordinates and can't be ignored, as the DwmDefWindowProc uses
    //  NCHITTEST  messages to determine how to highlight the caption buttons
    //  (the buttons then flicker as the hit tests sent by the user's mouse
    //  trigger different effects to those sent by the tooltip).
    //
    //  So instead, we have to partially implement tooltips ourselves using
    //  TTF_TRACKed tooltips.
    //
    // TODO(glen): Resolve this with Microsoft.
    class AeroTooltipManager : public TooltipManagerWin
    {
    public:
        explicit AeroTooltipManager(Widget* widget);
        virtual ~AeroTooltipManager();

        virtual void OnMouse(UINT u_msg, WPARAM w_param, LPARAM l_param);

    private:
        void OnTimer();

        class InitialTimer : public base::RefCounted<InitialTimer>
        {
        public:
            explicit InitialTimer(AeroTooltipManager* manager);
            void Start(int time);
            void Disown();
            void Execute();

        private:
            friend class base::RefCounted<InitialTimer>;

            ~InitialTimer() {}

            AeroTooltipManager* manager_;
        };

        int initial_delay_;
        scoped_refptr<InitialTimer> initial_timer_;
    };

} //namespace view

#endif //__view_aero_tooltip_manager_h__