
#include "window_manager.h"

#include "view/event/event.h"
#include "widget.h"

namespace
{

    view::WindowManager* window_manager = NULL;

    class NullWindowManager : public view::WindowManager
    {
    public:
        NullWindowManager() : mouse_capture_(NULL) {}

        virtual void StartMoveDrag(view::Widget* widget,
            const gfx::Point& screen_point)
        {
            NOTIMPLEMENTED();
        }

        virtual void StartResizeDrag(view::Widget* widget,
            const gfx::Point& screen_point,
            int hittest_code)
        {
            NOTIMPLEMENTED();
        }

        virtual bool SetMouseCapture(view::Widget* widget)
        {
            if(mouse_capture_ == widget)
            {
                return true;
            }
            if(mouse_capture_)
            {
                return false;
            }
            mouse_capture_ = widget;
            return true;
        }

        virtual bool ReleaseMouseCapture(view::Widget* widget)
        {
            if(widget && mouse_capture_!=widget)
            {
                return false;
            }
            mouse_capture_ = NULL;
            return true;
        }

        virtual bool HasMouseCapture(const view::Widget* widget) const
        {
            return mouse_capture_ == widget;
        }

        virtual bool HandleKeyEvent(view::Widget* widget,
            const view::KeyEvent& event)
        {
            return false;
        }

        virtual bool HandleMouseEvent(view::Widget* widget,
            const view::MouseEvent& event)
        {
            if(mouse_capture_)
            {
                view::MouseEvent translated(event, widget->GetRootView(),
                    mouse_capture_->GetRootView());
                mouse_capture_->OnMouseEvent(translated);
                return true;
            }
            return false;
        }

        void Register(view::Widget* widget) {}

    private:
        view::Widget* mouse_capture_;
    };

}

namespace view
{

    WindowManager::WindowManager() {}

    WindowManager::~WindowManager() {}

    // static
    void WindowManager::Install(WindowManager* wm)
    {
        window_manager = wm;
    }

    // static
    WindowManager* WindowManager::Get()
    {
        if(!window_manager)
        {
            window_manager = new NullWindowManager();
        }
        return window_manager;
    }

} //namespace view