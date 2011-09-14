
#include "dock_info.h"

#include "base/logging.h"
#include "base/win/scoped_gdi_object.h"

#include "ui_base/win/screen.h"

#include "browser_view.h"
#include "tab.h"

namespace
{

    // Distance in pixels between the hotspot and when the hint should be shown.
    const int kHotSpotDeltaX = 120;
    const int kHotSpotDeltaY = 120;

    // Size of the popup window.
    const int kPopupWidth = 70;
    const int kPopupHeight = 70;

}

// static
DockInfo::Factory* DockInfo::factory_ = NULL;

// static
bool DockInfo::IsCloseToPoint(const gfx::Point& screen_loc,
                              int x,
                              int y,
                              bool* in_enable_area)
{
    int delta_x = abs(x - screen_loc.x());
    int delta_y = abs(y - screen_loc.y());
    *in_enable_area = (delta_x < kPopupWidth / 2 && delta_y < kPopupHeight / 2);
    return *in_enable_area || (delta_x < kHotSpotDeltaX &&
        delta_y < kHotSpotDeltaY);
}

// static
bool DockInfo::IsCloseToMonitorPoint(const gfx::Point& screen_loc,
                                     int x,
                                     int y,
                                     DockInfo::Type type,
                                     bool* in_enable_area)
{
    // Because the monitor relative positions are aligned with the edge of the
    // monitor these need to be handled differently.
    int delta_x = abs(x - screen_loc.x());
    int delta_y = abs(y - screen_loc.y());

    int enable_delta_x = kPopupWidth / 2;
    int enable_delta_y = kPopupHeight / 2;
    int hot_spot_delta_x = kHotSpotDeltaX;
    int hot_spot_delta_y = kHotSpotDeltaY;

    switch(type)
    {
    case DockInfo::LEFT_HALF:
    case DockInfo::RIGHT_HALF:
        enable_delta_x += enable_delta_x;
        hot_spot_delta_x += hot_spot_delta_x;
        break;


    case DockInfo::MAXIMIZE:
        {
            // Make the maximize height smaller than the tab height to avoid showing
            // the dock indicator when close to maximized browser.
            hot_spot_delta_y = Tab::GetMinimumUnselectedSize().height() - 1;
            enable_delta_y = hot_spot_delta_y / 2;
            break;
        }

    case DockInfo::BOTTOM_HALF:
        enable_delta_y += enable_delta_y;
        hot_spot_delta_y += hot_spot_delta_y;
        break;

    default:
        NOTREACHED();
        return false;
    }
    *in_enable_area = (delta_x < enable_delta_x && delta_y < enable_delta_y);
    bool result = (*in_enable_area || (delta_x < hot_spot_delta_x &&
        delta_y < hot_spot_delta_y));
    if(type != DockInfo::MAXIMIZE)
    {
        return result;
    }

    // Make the hot spot/enable spot for maximized windows the whole top of the
    // monitor.
    int max_delta_y = abs(screen_loc.y() - y);
    *in_enable_area = (*in_enable_area || (max_delta_y < enable_delta_y));
    return *in_enable_area || (max_delta_y < hot_spot_delta_y);
}

// static
int DockInfo::popup_width()
{
    return kPopupWidth;
}

// static
int DockInfo::popup_height()
{
    return kPopupHeight;
}

bool DockInfo::IsValidForPoint(const gfx::Point& screen_point)
{
    if(type_ == NONE)
    {
        return false;
    }

    if(window_)
    {
        return IsCloseToPoint(screen_point, hot_spot_.x(), hot_spot_.y(),
            &in_enable_area_);
    }

    return monitor_bounds_.Contains(screen_point) &&
        IsCloseToMonitorPoint(screen_point, hot_spot_.x(),
        hot_spot_.y(), type_, &in_enable_area_);
}

bool DockInfo::GetNewWindowBounds(gfx::Rect* new_window_bounds,
                                  bool* maximize_new_window) const
{
    if(type_==NONE || !in_enable_area_)
    {
        return false;
    }

    gfx::Rect window_bounds;
    if(window_ && !GetWindowBounds(&window_bounds))
    {
        return false;
    }

    int half_m_width = (monitor_bounds_.right() - monitor_bounds_.x()) / 2;
    int half_m_height = (monitor_bounds_.bottom() - monitor_bounds_.y()) / 2;

    *maximize_new_window = false;

    switch(type_)
    {
    case LEFT_OF_WINDOW:
        new_window_bounds->SetRect(monitor_bounds_.x(), window_bounds.y(),
            half_m_width, window_bounds.height());
        break;

    case RIGHT_OF_WINDOW:
        new_window_bounds->SetRect(monitor_bounds_.x() + half_m_width,
            window_bounds.y(), half_m_width,
            window_bounds.height());
        break;

    case TOP_OF_WINDOW:
        new_window_bounds->SetRect(window_bounds.x(), monitor_bounds_.y(),
            window_bounds.width(), half_m_height);
        break;

    case BOTTOM_OF_WINDOW:
        new_window_bounds->SetRect(window_bounds.x(),
            monitor_bounds_.y() + half_m_height,
            window_bounds.width(), half_m_height);
        break;

    case LEFT_HALF:
        new_window_bounds->SetRect(monitor_bounds_.x(), monitor_bounds_.y(),
            half_m_width, monitor_bounds_.height());
        break;

    case RIGHT_HALF:
        new_window_bounds->SetRect(monitor_bounds_.right() - half_m_width,
            monitor_bounds_.y(), half_m_width, monitor_bounds_.height());
        break;

    case BOTTOM_HALF:
        new_window_bounds->SetRect(monitor_bounds_.x(),
            monitor_bounds_.y() + half_m_height,
            monitor_bounds_.width(), half_m_height);
        break;

    case MAXIMIZE:
        *maximize_new_window = true;
        break;

    default:
        NOTREACHED();
    }
    return true;
}

void DockInfo::AdjustOtherWindowBounds() const
{
    if(!in_enable_area_)
    {
        return;
    }

    gfx::Rect window_bounds;
    if(!window_ || !GetWindowBounds(&window_bounds))
    {
        return;
    }

    gfx::Rect other_window_bounds;
    int half_m_width = (monitor_bounds_.right() - monitor_bounds_.x()) / 2;
    int half_m_height = (monitor_bounds_.bottom() - monitor_bounds_.y()) / 2;

    switch(type_)
    {
    case LEFT_OF_WINDOW:
        other_window_bounds.SetRect(monitor_bounds_.x() + half_m_width,
            window_bounds.y(), half_m_width,
            window_bounds.height());
        break;

    case RIGHT_OF_WINDOW:
        other_window_bounds.SetRect(monitor_bounds_.x(), window_bounds.y(),
            half_m_width, window_bounds.height());
        break;

    case TOP_OF_WINDOW:
        other_window_bounds.SetRect(window_bounds.x(),
            monitor_bounds_.y() + half_m_height,
            window_bounds.width(), half_m_height);
        break;

    case BOTTOM_OF_WINDOW:
        other_window_bounds.SetRect(window_bounds.x(), monitor_bounds_.y(),
            window_bounds.width(), half_m_height);
        break;

    default:
        return;
    }

    SizeOtherWindowTo(other_window_bounds);
}

gfx::Rect DockInfo::GetPopupRect() const
{
    int x = hot_spot_.x() - popup_width() / 2;
    int y = hot_spot_.y() - popup_height() / 2;
    switch(type_)
    {
    case LEFT_OF_WINDOW:
    case RIGHT_OF_WINDOW:
    case TOP_OF_WINDOW:
    case BOTTOM_OF_WINDOW:
        {
            // Constrain the popup to the monitor's bounds.
            gfx::Rect ideal_bounds(x, y, popup_width(), popup_height());
            ideal_bounds = ideal_bounds.AdjustToFit(monitor_bounds_);
            return ideal_bounds;
        }
    case DockInfo::MAXIMIZE:
        y += popup_height() / 2;
        break;
    case DockInfo::LEFT_HALF:
        x += popup_width() / 2;
        break;
    case DockInfo::RIGHT_HALF:
        x -= popup_width() / 2;
        break;
    case DockInfo::BOTTOM_HALF:
        y -= popup_height() / 2;
        break;

    default:
        NOTREACHED();
    }
    return gfx::Rect(x, y, popup_width(), popup_height());
}

bool DockInfo::CheckMonitorPoint(const gfx::Point& screen_loc,
                                 int x,
                                 int y,
                                 Type type)
{
    if(IsCloseToMonitorPoint(screen_loc, x, y, type, &in_enable_area_))
    {
        hot_spot_.SetPoint(x, y);
        type_ = type;
        return true;
    }
    return false;
}


namespace
{

    // BaseWindowFinder -----------------------------------------------------------

    // Base class used to locate a window. This is intended to be used with the
    // various win32 functions that iterate over windows.
    //
    // A subclass need only override ShouldStopIterating to determine when
    // iteration should stop.
    class BaseWindowFinder
    {
    public:
        // Creates a BaseWindowFinder with the specified set of HWNDs to ignore.
        explicit BaseWindowFinder(const std::set<HWND>& ignore) : ignore_(ignore) {}
        virtual ~BaseWindowFinder() {}

    protected:
        // Returns true if iteration should stop, false if iteration should continue.
        virtual bool ShouldStopIterating(HWND window) = 0;

        static BOOL CALLBACK WindowCallbackProc(HWND hwnd, LPARAM lParam)
        {
            BaseWindowFinder* finder = reinterpret_cast<BaseWindowFinder*>(lParam);
            if(finder->ignore_.find(hwnd) != finder->ignore_.end())
            {
                return TRUE;
            }

            return finder->ShouldStopIterating(hwnd) ? FALSE : TRUE;
        }

    private:
        const std::set<HWND>& ignore_;

        DISALLOW_COPY_AND_ASSIGN(BaseWindowFinder);
    };

    // TopMostFinder --------------------------------------------------------------

    // Helper class to determine if a particular point of a window is not obscured
    // by another window.
    class TopMostFinder : public BaseWindowFinder
    {
    public:
        // Returns true if |window| is the topmost window at the location
        // |screen_loc|, not including the windows in |ignore|.
        static bool IsTopMostWindowAtPoint(HWND window,
            const gfx::Point& screen_loc,
            const std::set<HWND>& ignore)
        {
            TopMostFinder finder(window, screen_loc, ignore);
            return finder.is_top_most_;
        }

        virtual bool ShouldStopIterating(HWND hwnd)
        {
            if(hwnd == target_)
            {
                // Window is topmost, stop iterating.
                is_top_most_ = true;
                return true;
            }

            if(!IsWindowVisible(hwnd))
            {
                // The window isn't visible, keep iterating.
                return false;
            }

            RECT r;
            if(!GetWindowRect(hwnd, &r) || !PtInRect(&r, screen_loc_.ToPOINT()))
            {
                // The window doesn't contain the point, keep iterating.
                return false;
            }

            LONG ex_styles = GetWindowLong(hwnd, GWL_EXSTYLE);
            if(ex_styles & WS_EX_TRANSPARENT || ex_styles & WS_EX_LAYERED)
            {
                // Mouse events fall through WS_EX_TRANSPARENT windows, so we ignore them.
                //
                // WS_EX_LAYERED is trickier. Apps like Switcher create a totally
                // transparent WS_EX_LAYERED window that is always on top. If we don't
                // ignore WS_EX_LAYERED windows and there are totally transparent
                // WS_EX_LAYERED windows then there are effectively holes on the screen
                // that the user can't reattach tabs to. So we ignore them. This is a bit
                // problematic in so far as WS_EX_LAYERED windows need not be totally
                // transparent in which case we treat chrome windows as not being obscured
                // when they really are, but this is better than not being able to
                // reattach tabs.
                return false;
            }

            // hwnd is at the point. Make sure the point is within the windows region.
            if(GetWindowRgn(hwnd, tmp_region_.Get()) == ERROR)
            {
                // There's no region on the window and the window contains the point. Stop
                // iterating.
                return true;
            }

            // The region is relative to the window's rect.
            BOOL is_point_in_region = PtInRegion(tmp_region_.Get(),
                screen_loc_.x() - r.left, screen_loc_.y() - r.top);
            tmp_region_ = CreateRectRgn(0, 0, 0, 0);
            // Stop iterating if the region contains the point.
            return !!is_point_in_region;
        }

    private:
        TopMostFinder(HWND window,
            const gfx::Point& screen_loc,
            const std::set<HWND>& ignore)
            : BaseWindowFinder(ignore),
            target_(window),
            screen_loc_(screen_loc),
            is_top_most_(false),
            tmp_region_(CreateRectRgn(0, 0, 0, 0))
        {
            EnumWindows(WindowCallbackProc, reinterpret_cast<LPARAM>(this));
        }

        // The window we're looking for.
        HWND target_;

        // Location of window to find.
        gfx::Point screen_loc_;

        // Is target_ the top most window? This is initially false but set to true
        // in ShouldStopIterating if target_ is passed in.
        bool is_top_most_;

        base::win::ScopedRegion tmp_region_;

        DISALLOW_COPY_AND_ASSIGN(TopMostFinder);
    };

    // WindowFinder ---------------------------------------------------------------

    // Helper class to determine if a particular point contains a window from our
    // process.
    class LocalProcessWindowFinder : public BaseWindowFinder
    {
    public:
        // Returns the hwnd from our process at screen_loc that is not obscured by
        // another window. Returns NULL otherwise.
        static HWND GetProcessWindowAtPoint(const gfx::Point& screen_loc,
            const std::set<HWND>& ignore)
        {
            LocalProcessWindowFinder finder(screen_loc, ignore);
            if(finder.result_ &&
                TopMostFinder::IsTopMostWindowAtPoint(finder.result_, screen_loc,
                ignore))
            {
                return finder.result_;
            }
            return NULL;
        }

    protected:
        virtual bool ShouldStopIterating(HWND hwnd)
        {
            RECT r;
            if(IsWindowVisible(hwnd) && GetWindowRect(hwnd, &r) &&
                PtInRect(&r, screen_loc_.ToPOINT()))
            {
                result_ = hwnd;
                return true;
            }
            return false;
        }

    private:
        LocalProcessWindowFinder(const gfx::Point& screen_loc,
            const std::set<HWND>& ignore)
            : BaseWindowFinder(ignore),
            screen_loc_(screen_loc),
            result_(NULL)
        {
            EnumThreadWindows(GetCurrentThreadId(), WindowCallbackProc,
                reinterpret_cast<LPARAM>(this));
        }

        // Position of the mouse.
        gfx::Point screen_loc_;

        // The resulting window. This is initially null but set to true in
        // ShouldStopIterating if an appropriate window is found.
        HWND result_;

        DISALLOW_COPY_AND_ASSIGN(LocalProcessWindowFinder);
    };

    // DockToWindowFinder ---------------------------------------------------------

    // Helper class for creating a DockInfo from a specified point.
    class DockToWindowFinder : public BaseWindowFinder
    {
    public:
        // Returns the DockInfo for the specified point. If there is no docking
        // position for the specified point the returned DockInfo has a type of NONE.
        static DockInfo GetDockInfoAtPoint(const gfx::Point& screen_loc,
            const std::set<HWND>& ignore)
        {
            DockToWindowFinder finder(screen_loc, ignore);
            if(!finder.result_.window() ||
                !TopMostFinder::IsTopMostWindowAtPoint(finder.result_.window(),
                finder.result_.hot_spot(),
                ignore))
            {
                finder.result_.set_type(DockInfo::NONE);
            }
            return finder.result_;
        }

    protected:
        virtual bool ShouldStopIterating(HWND hwnd)
        {
            BrowserView* window = BrowserView::GetBrowserViewForNativeWindow(hwnd);
            RECT bounds;
            if(!window || !IsWindowVisible(hwnd) || !GetWindowRect(hwnd, &bounds))
            {
                return false;
            }

            // Check the three corners we allow docking to. We don't currently allow
            // docking to top of window as it conflicts with docking to the tab strip.
            if(CheckPoint(hwnd, bounds.left, (bounds.top + bounds.bottom) / 2,
                DockInfo::LEFT_OF_WINDOW) ||
                CheckPoint(hwnd, bounds.right - 1, (bounds.top + bounds.bottom) / 2,
                DockInfo::RIGHT_OF_WINDOW) ||
                CheckPoint(hwnd, (bounds.left + bounds.right) / 2, bounds.bottom - 1,
                DockInfo::BOTTOM_OF_WINDOW))
            {
                return true;
            }
            return false;
        }

    private:
        DockToWindowFinder(const gfx::Point& screen_loc,
            const std::set<HWND>& ignore)
            : BaseWindowFinder(ignore),
            screen_loc_(screen_loc)
        {
            gfx::Rect work_area = ui::Screen::GetMonitorWorkAreaNearestPoint(
                screen_loc);
            if(!work_area.IsEmpty())
            {
                result_.set_monitor_bounds(work_area);
                EnumThreadWindows(GetCurrentThreadId(), WindowCallbackProc,
                    reinterpret_cast<LPARAM>(this));
            }
        }

        bool CheckPoint(HWND hwnd, int x, int y, DockInfo::Type type)
        {
            bool in_enable_area;
            if(DockInfo::IsCloseToPoint(screen_loc_, x, y, &in_enable_area))
            {
                result_.set_in_enable_area(in_enable_area);
                result_.set_window(hwnd);
                result_.set_type(type);
                result_.set_hot_spot(gfx::Point(x, y));
                // Only show the hotspot if the monitor contains the bounds of the popup
                // window. Otherwise we end with a weird situation where the popup window
                // isn't completely visible.
                return result_.monitor_bounds().Contains(result_.GetPopupRect());
            }
            return false;
        }

        // The location to look for.
        gfx::Point screen_loc_;

        // The resulting DockInfo.
        DockInfo result_;

        DISALLOW_COPY_AND_ASSIGN(DockToWindowFinder);
    };

}

// DockInfo -------------------------------------------------------------------

// static
DockInfo DockInfo::GetDockInfoAtPoint(const gfx::Point& screen_point,
                                      const std::set<HWND>& ignore)
{
    if(factory_)
    {
        return factory_->GetDockInfoAtPoint(screen_point, ignore);
    }

    // Try docking to a window first.
    DockInfo info = DockToWindowFinder::GetDockInfoAtPoint(screen_point, ignore);
    if(info.type() != DockInfo::NONE)
    {
        return info;
    }

    // No window relative positions. Try monitor relative positions.
    const gfx::Rect& m_bounds = info.monitor_bounds();
    int mid_x = m_bounds.x() + m_bounds.width() / 2;
    int mid_y = m_bounds.y() + m_bounds.height() / 2;

    bool result =
        info.CheckMonitorPoint(screen_point, mid_x, m_bounds.y(),
        DockInfo::MAXIMIZE) ||
        info.CheckMonitorPoint(screen_point, mid_x, m_bounds.bottom(),
        DockInfo::BOTTOM_HALF) ||
        info.CheckMonitorPoint(screen_point, m_bounds.x(), mid_y,
        DockInfo::LEFT_HALF) ||
        info.CheckMonitorPoint(screen_point, m_bounds.right(), mid_y,
        DockInfo::RIGHT_HALF);

    return info;
}

HWND DockInfo::GetLocalProcessWindowAtPoint(const gfx::Point& screen_point,
                                            const std::set<HWND>& ignore)
{
    if(factory_)
    {
        return factory_->GetLocalProcessWindowAtPoint(screen_point, ignore);
    }
    return LocalProcessWindowFinder::GetProcessWindowAtPoint(
        screen_point, ignore);
}

bool DockInfo::GetWindowBounds(gfx::Rect* bounds) const
{
    RECT window_rect;
    if(!window() || !GetWindowRect(window(), &window_rect))
    {
        return false;
    }
    *bounds = gfx::Rect(window_rect);
    return true;
}

void DockInfo::SizeOtherWindowTo(const gfx::Rect& bounds) const
{
    if(IsZoomed(window()))
    {
        // We're docking relative to another window, we need to make sure the
        // window we're docking to isn't maximized.
        ShowWindow(window(), SW_RESTORE|SW_SHOWNA);
    }
    SetWindowPos(window(), HWND_TOP, bounds.x(), bounds.y(), bounds.width(),
        bounds.height(), SWP_NOACTIVATE|SWP_NOOWNERZORDER);
}