
#include "desktop_window_view.h"

#include "ui_gfx/canvas.h"
#include "ui_gfx/transform.h"

#include "desktop_background.h"
#include "desktop_window_root_view.h"

#include "view/layer_property_setter.h"
#include "view/widget/native_widget_view.h"
#include "view/widget/native_widget_views.h"
#include "view/widget/native_widget_win.h"
#include "view/widget/widget.h"

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

        DesktopWindowView::DesktopWindowView() : active_widget_(NULL)
        {
            set_background(new DesktopBackground);
        }

        DesktopWindowView::~DesktopWindowView() {}

        // static
        void DesktopWindowView::CreateDesktopWindow()
        {
            DCHECK(!desktop_window_view);
            desktop_window_view = new DesktopWindowView;
            Widget* window = new DesktopWindow(desktop_window_view);
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

            if(active_widget_)
            {
                active_widget_->OnActivate(false);
            }
            if(widget)
            {
                widget->MoveToTop();
                active_widget_ = static_cast<NativeWidgetViews*>(widget->native_widget());
                active_widget_->OnActivate(true);
            }
        }


        void DesktopWindowView::CreateTestWindow(const std::wstring& title,
            SkColor color,
            gfx::Rect initial_bounds,
            bool rotate)
        {
            Widget* window = Widget::CreateWindowWithBounds(
                new TestWindowContentView(title, color),
                initial_bounds);
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

        ////////////////////////////////////////////////////////////////////////////////
        // DesktopWindowView, WidgetDelegate implementation:

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

    } //namespace desktop
} //namespace view