
#include "desktop_window_view.h"

#include "ui_gfx/canvas.h"
#include "ui_gfx/transform.h"

#include "desktop_background.h"
#include "desktop_window_manager.h"
#include "desktop_window_root_view.h"

#include "view/layer_property_setter.h"
#include "view/widget/native_widget_view.h"
#include "view/widget/native_widget_views.h"
#include "view/widget/native_widget_win.h"
#include "view/widget/widget.h"
#include "view/window/native_frame_view.h"

namespace view
{
    namespace desktop
    {

        // The Widget that hosts the DesktopWindowView. Subclasses Widget to override
        // CreateRootView() so that the DesktopWindowRootView can be supplied instead
        // for custom event filtering.
        class DesktopWindow : public Widget
        {
        public:
            explicit DesktopWindow(DesktopWindowView* desktop_window_view)
                : desktop_window_view_(desktop_window_view) {}
            virtual ~DesktopWindow() {}

        private:
            // Overridden from Widget:
            virtual internal::RootView* CreateRootView()
            {
                return new DesktopWindowRootView(desktop_window_view_, this);
            }

            virtual bool OnKeyEvent(const KeyEvent& event)
            {
                NativeWidgetViews* native_widget =
                    desktop_window_view_->active_native_widget();
                return native_widget ? native_widget->OnKeyEvent(event) : false;
            }

            virtual bool OnMouseEvent(const MouseEvent& event)
            {
                if(event.type() == ui::ET_MOUSEWHEEL)
                {
                    NativeWidgetViews* native_widget =
                        desktop_window_view_->active_native_widget();
                    if(native_widget)
                    {
                        return native_widget->delegate()->OnMouseEvent(event);
                    }
                }
                return WindowManager::Get()->HandleMouseEvent(this, event) ||
                    Widget::OnMouseEvent(event);
            }

            DesktopWindowView* desktop_window_view_;

            DISALLOW_COPY_AND_ASSIGN(DesktopWindow);
        };

        class TestWindowContentView : public WidgetDelegateView
        {
        public:
            TestWindowContentView(const std::wstring& title, SkColor color)
                : title_(title), color_(color) {}
            virtual ~TestWindowContentView() {}

        private:
            // Overridden from View:
            virtual void OnPaint(gfx::Canvas* canvas)
            {
                canvas->FillRectInt(color_, 0, 0, width(), height());
            }

            // Overridden from WindowDelegate:
            virtual std::wstring GetWindowTitle() const
            {
                return title_;
            }
            virtual View* GetContentsView()
            {
                return this;
            }
            virtual bool CanMaximize() const
            {
                return true;
            }
            virtual bool OnMousePressed(const MouseEvent& event)
            {
                Widget* widget = View::GetWidget();
                if(widget->IsMinimized())
                {
                    widget->Restore();
                }
                else
                {
                    widget->Minimize();
                }
                return true;
            }

            std::wstring title_;
            SkColor color_;

            DISALLOW_COPY_AND_ASSIGN(TestWindowContentView);
        };

        ////////////////////////////////////////////////////////////////////////////////
        // DesktopWindowView, public:

        // static
        DesktopWindowView* DesktopWindowView::desktop_window_view = NULL;

        DesktopWindowView::DesktopWindowView(DesktopType type)
            : active_native_widget_(NULL), type_(type)
        {
            switch(type_)
            {
            case DESKTOP_DEFAULT:
            case DESKTOP_NETBOOK:
                set_background(new DesktopBackground());
                break;
            case DESKTOP_OTHER:
                set_background(Background::CreateStandardPanelBackground());
                break;
            }
        }

        DesktopWindowView::~DesktopWindowView() {}

        // static
        void DesktopWindowView::CreateDesktopWindow(DesktopType type)
        {
            DCHECK(!desktop_window_view);
            desktop_window_view = new DesktopWindowView(type);
            Widget* window = new DesktopWindow(desktop_window_view);
            desktop_window_view->widget_ = window;

            WindowManager::Install(new DesktopWindowManager(window));

            Widget::InitParams params;
            params.delegate = desktop_window_view;
            // In this environment, CreateChromeWindow will default to creating a views-
            // window, so we need to construct a NativeWidgetWin by hand.
            // TODO(beng): Replace this with NativeWindow::CreateNativeRootWindow().
            params.native_widget = new NativeWidgetWin(window);
            params.bounds = gfx::Rect(20, 20, 1920, 1200);
            
            window->Init(params);
            window->Show();
        }

        void DesktopWindowView::ActivateWidget(Widget* widget)
        {
            if(widget && widget->IsActive())
            {
                return;
            }

            if(widget)
            {
                if(!widget->HasObserver(this))
                {
                    widget->AddObserver(this);
                }
                widget->Activate();
            }
        }

        void DesktopWindowView::CreateTestWindow(const std::wstring& title,
            SkColor color,
            gfx::Rect initial_bounds,
            bool rotate)
        {
            Widget* window = Widget::CreateWindowWithBounds(
                new TestWindowContentView(title, color), initial_bounds);
            window->Show();

            if(rotate)
            {
                gfx::Transform transform;
                transform.SetRotate(90.0f);
                transform.SetTranslateX(window->GetWindowScreenBounds().width());
                static_cast<NativeWidgetViews*>(window->native_widget())->GetView()->
                    SetTransform(transform);
            }
            static_cast<NativeWidgetViews*>(window->native_widget())->GetView()->
                SetLayerPropertySetter(LayerPropertySetter::CreateAnimatingSetter());
        }

        ////////////////////////////////////////////////////////////////////////////////
        // DesktopWindowView, View overrides:

        void DesktopWindowView::Layout() {}

        void DesktopWindowView::ViewHierarchyChanged(
            bool is_add, View* parent, View* child)
        {
            if(!is_add && active_native_widget_ &&
                active_native_widget_->GetView()==child)
            {
                active_native_widget_ = NULL;
            }
            else if(is_add &&
                child->GetClassName()==internal::NativeWidgetView::kViewClassName)
            {
                internal::NativeWidgetView* native_widget_view =
                    static_cast<internal::NativeWidgetView*>(child);
                native_widget_view->GetAssociatedWidget()->AddObserver(this);
            }
        }

        ////////////////////////////////////////////////////////////////////////////////
        // DesktopWindowView, WidgetDelegate implementation:

        Widget* DesktopWindowView::GetWidget()
        {
            return widget_;
        }

        const Widget* DesktopWindowView::GetWidget() const
        {
            return widget_;
        }

        bool DesktopWindowView::CanResize() const
        {
            return true;
        }

        bool DesktopWindowView::CanMaximize() const
        {
            return CanResize();
        }

        std::wstring DesktopWindowView::GetWindowTitle() const
        {
            return L"Aura Desktop";
        }

        SkBitmap DesktopWindowView::GetWindowAppIcon()
        {
            return SkBitmap();
        }

        SkBitmap DesktopWindowView::GetWindowIcon()
        {
            return SkBitmap();
        }

        bool DesktopWindowView::ShouldShowWindowIcon() const
        {
            return false;
        }

        void DesktopWindowView::WindowClosing()
        {
            MessageLoopForUI::current()->Quit();
        }

        View* DesktopWindowView::GetContentsView()
        {
            return this;
        }

        NonClientFrameView* DesktopWindowView::CreateNonClientFrameView()
        {
            switch(type_)
            {
            case DESKTOP_DEFAULT:
            case DESKTOP_NETBOOK:
                return NULL;

            case DESKTOP_OTHER:
                return new NativeFrameView(widget_);
            }
            return NULL;
        }

        void DesktopWindowView::OnWidgetClosing(Widget* widget)
        {
            if(active_native_widget_ && static_cast<internal::NativeWidgetPrivate*>
                (active_native_widget_)->GetWidget() == widget)
            {
                active_native_widget_ = NULL;
            }
        }

        void DesktopWindowView::OnWidgetVisibilityChanged(Widget* widget,
            bool visible) {}

        void DesktopWindowView::OnWidgetActivationChanged(Widget* widget,
            bool active)
        {
            if(active)
            {
                if(active_native_widget_)
                {
                    active_native_widget_->GetWidget()->Deactivate();
                }
                active_native_widget_ =
                    static_cast<NativeWidgetViews*>(widget->native_widget());
            }
            else if(widget == active_native_widget_->GetWidget())
            {
                active_native_widget_ = NULL;
            }
        }

    } //namespace desktop
} //namespace view