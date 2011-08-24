
#include "native_control.h"

#include <atlbase.h>
#include <atlwin.h>

#include "ui_base/keycodes/keyboard_code_conversion_win.h"
#include "ui_base/l10n/l10n_util_win.h"
#include "ui_base/view_prop.h"
#include "ui_base/win/hwnd_util.h"

#include "native/native_view_host.h"
#include "view/message_crack.h"
#include "view/widget/widget.h"

#define VIEW_DECLARE_FRAME_WND_CLASS(WndClassName) \
static ATL::CWndClassInfo& GetWndClassInfo() \
{ \
    static ATL::CWndClassInfo wc = \
    { \
        { sizeof(WNDCLASSEX), 0, StartWindowProc, \
        0, 0, NULL, NULL, NULL, (HBRUSH)(COLOR_WINDOW + 1), NULL, WndClassName, NULL }, \
        NULL, NULL, IDC_ARROW, TRUE, 0, _T("") \
    }; \
    return wc; \
}

namespace view
{

    // Maps to the NativeControl.
    static const char* const kNativeControlKey = "__NATIVE_CONTROL__";

    class NativeControlContainer : public CWindowImpl<NativeControlContainer,
        CWindow, CWinTraits<WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN> >
    {
    public:
        explicit NativeControlContainer(NativeControl* parent)
            : parent_(parent), control_(NULL), original_handler_(NULL) {}

        void Init()
        {
            Create(parent_->GetWidget()->GetNativeView());
            ::ShowWindow(m_hWnd, SW_SHOW);
        }

        virtual ~NativeControlContainer() {}

        // NOTE: If you add a new message, be sure and verify parent_ is valid before
        // calling into parent_.
        VIEW_DECLARE_FRAME_WND_CLASS(L"WanViewNativeControlContainer");
        VIEW_BEGIN_MSG_MAP_EX(NativeControlContainer);
            VIEW_MSG_WM_CREATE(OnCreate);
            VIEW_MSG_WM_ERASEBKGND(OnEraseBkgnd);
            VIEW_MSG_WM_PAINT(OnPaint);
            VIEW_MSG_WM_SIZE(OnSize);
            VIEW_MSG_WM_NOTIFY(OnNotify);
            VIEW_MSG_WM_COMMAND(OnCommand);
            VIEW_MSG_WM_DESTROY(OnDestroy);
            VIEW_MSG_WM_CONTEXTMENU(OnContextMenu);
            VIEW_MSG_WM_CTLCOLORBTN(OnCtlColorBtn);
            VIEW_MSG_WM_CTLCOLORSTATIC(OnCtlColorStatic)
        VIEW_END_MSG_MAP();

        HWND GetControl()
        {
            return control_;
        }

        // Called when the parent is getting deleted. This control stays around until
        // it gets the OnFinalMessage call.
        void ResetParent()
        {
            parent_ = NULL;
        }

        void OnFinalMessage(HWND hwnd)
        {
            if(parent_)
            {
                parent_->NativeControlDestroyed();
            }
            delete this;
        }

    private:
        friend class NativeControl;

        LRESULT OnCreate(LPCREATESTRUCT create_struct)
        {
            control_ = parent_->CreateNativeControl(m_hWnd);

            // We subclass the control hwnd so we get the WM_KEYDOWN messages.
            original_handler_ = ui::SetWindowProc(control_,
                &NativeControl::NativeControlWndProc);
            prop_.reset(new ui::ViewProp(control_, kNativeControlKey , parent_));

            ::ShowWindow(control_, SW_SHOW);
            return 1;
        }

        LRESULT OnEraseBkgnd(HDC dc)
        {
            return 1;
        }

        void OnPaint(HDC ignore)
        {
            PAINTSTRUCT ps;
            HDC dc = ::BeginPaint(*this, &ps);
            ::EndPaint(*this, &ps);
        }

        void OnSize(int type, const gfx::Size& sz)
        {
            ::MoveWindow(control_, 0, 0, sz.width(), sz.height(), TRUE);
        }

        LRESULT OnCommand(UINT code, int id, HWND source)
        {
            return parent_ ? parent_->OnCommand(code, id, source) : 0;
        }

        LRESULT OnNotify(int w_param, LPNMHDR l_param)
        {
            if(parent_)
            {
                return parent_->OnNotify(w_param, l_param);
            }
            else
            {
                return 0;
            }
        }

        void OnDestroy()
        {
            if(parent_)
            {
                parent_->OnDestroy();
            }
        }

        void OnContextMenu(HWND window, const gfx::Point& location)
        {
            if(parent_)
            {
                parent_->OnContextMenu(location.ToPOINT());
            }
        }

        // We need to find an ancestor with a non-null background, and
        // ask it for a (solid color) brush that approximates
        // the background.  The caller will use this when drawing
        // the native control as a background color, particularly
        // for radiobuttons and XP style pushbuttons.
        LRESULT OnCtlColor(UINT msg, HDC dc, HWND control)
        {
            const View* ancestor = parent_;
            while(ancestor)
            {
                const Background* background = ancestor->background();
                if(background)
                {
                    HBRUSH brush = background->GetNativeControlBrush();
                    if(brush)
                    {
                        return reinterpret_cast<LRESULT>(brush);
                    }
                }
                ancestor = ancestor->parent();
            }

            // COLOR_BTNFACE is the default for dialog box backgrounds.
            return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
        }

        LRESULT OnCtlColorBtn(HDC dc, HWND control)
        {
            return OnCtlColor(WM_CTLCOLORBTN, dc, control);
        }

        LRESULT OnCtlColorStatic(HDC dc, HWND control)
        {
            return OnCtlColor(WM_CTLCOLORSTATIC, dc, control);
        }

        NativeControl* parent_;
        HWND control_;

        // Message handler that was set before we reset it.
        WNDPROC original_handler_;

        scoped_ptr<ui::ViewProp> prop_;

        DISALLOW_COPY_AND_ASSIGN(NativeControlContainer);
    };

    NativeControl::NativeControl() : hwnd_view_(NULL),
        container_(NULL),
        fixed_width_(-1),
        horizontal_alignment_(CENTER),
        fixed_height_(-1),
        vertical_alignment_(CENTER)
    {
        set_focusable(true);
    }

    NativeControl::~NativeControl()
    {
        if(container_)
        {
            container_->ResetParent();
            ::DestroyWindow(*container_);
        }
    }

    void NativeControl::ValidateNativeControl()
    {
        if(hwnd_view_ == NULL)
        {
            hwnd_view_ = new NativeViewHost;
            AddChildView(hwnd_view_);
        }

        if(!container_ && IsVisible())
        {
            container_ = new NativeControlContainer(this);
            container_->Init();
            hwnd_view_->Attach(*container_);
            if(!IsEnabled())
            {
                EnableWindow(GetNativeControlHWND(), IsEnabled());
            }

            // This message ensures that the focus border is shown.
            ::SendMessage(container_->GetControl(),
                WM_CHANGEUISTATE,
                MAKELPARAM(UIS_CLEAR, UISF_HIDEFOCUS),
                0);
        }
    }

    void NativeControl::ViewHierarchyChanged(bool is_add, View* parent,
        View* child)
    {
        if(is_add && parent!=this && !container_ && GetWidget())
        {
            ValidateNativeControl();
            Layout();
        }
    }

    void NativeControl::Layout()
    {
        if(!container_ && GetWidget())
        {
            ValidateNativeControl();
        }

        if(hwnd_view_)
        {
            gfx::Rect lb = GetLocalBounds();

            int x = lb.x();
            int y = lb.y();
            int width = lb.width();
            int height = lb.height();
            if(fixed_width_ > 0)
            {
                width = std::min(fixed_width_, width);
                switch(horizontal_alignment_)
                {
                case LEADING:
                    // Nothing to do.
                    break;
                case CENTER:
                    x += (lb.width() - width) / 2;
                    break;
                case TRAILING:
                    x = x + lb.width() - width;
                    break;
                default:
                    NOTREACHED();
                }
            }

            if(fixed_height_ > 0)
            {
                height = std::min(fixed_height_, height);
                switch(vertical_alignment_)
                {
                case LEADING:
                    // Nothing to do.
                    break;
                case CENTER:
                    y += (lb.height() - height) / 2;
                    break;
                case TRAILING:
                    y = y + lb.height() - height;
                    break;
                default:
                    NOTREACHED();
                }
            }

            hwnd_view_->SetBounds(x, y, width, height);
        }
    }

    void NativeControl::OnContextMenu(const POINT& location)
    {
        if(!context_menu_controller())
        {
            return;
        }

        if(location.x==-1 && location.y==-1)
        {
            ShowContextMenu(GetKeyboardContextMenuLocation(), false);
        }
        else
        {
            ShowContextMenu(gfx::Point(location), true);
        }
    }

    void NativeControl::OnFocus()
    {
        if(container_)
        {
            DCHECK(container_->GetControl());
            ::SetFocus(container_->GetControl());
            if(GetWidget())
            {
                GetWidget()->NotifyAccessibilityEvent(
                    this, ui::AccessibilityTypes::EVENT_FOCUS, false);
            }
        }
    }

    HWND NativeControl::GetNativeControlHWND()
    {
        if(container_)
        {
            return container_->GetControl();
        }
        else
        {
            return NULL;
        }
    }

    void NativeControl::NativeControlDestroyed()
    {
        if(hwnd_view_)
        {
            hwnd_view_->Detach();
        }
        container_ = NULL;
    }

    void NativeControl::SetVisible(bool f)
    {
        if(f != IsVisible())
        {
            View::SetVisible(f);
            if(!f && container_)
            {
                ::DestroyWindow(*container_);
            }
            else if(f && !container_)
            {
                ValidateNativeControl();
            }
        }
    }

    void NativeControl::OnEnabledChanged()
    {
        View::OnEnabledChanged();
        if(GetNativeControlHWND())
        {
            EnableWindow(GetNativeControlHWND(), IsEnabled());
        }
    }

    void NativeControl::OnPaint(gfx::Canvas* canvas) {}

    void NativeControl::VisibilityChanged(View* starting_from, bool is_visible)
    {
        SetVisible(is_visible);
    }

    void NativeControl::SetFixedWidth(int width, Alignment alignment)
    {
        DCHECK_GT(width, 0);
        fixed_width_ = width;
        horizontal_alignment_ = alignment;
    }

    void NativeControl::SetFixedHeight(int height, Alignment alignment)
    {
        DCHECK_GT(height, 0);
        fixed_height_ = height;
        vertical_alignment_ = alignment;
    }

    DWORD NativeControl::GetAdditionalExStyle() const
    {
        // If the UI for the view is mirrored, we should make sure we add the
        // extended window style for a right-to-left layout so the subclass creates
        // a mirrored HWND for the underlying control.
        DWORD ex_style = 0;
        if(base::i18n::IsRTL())
        {
            ex_style |= ui::GetExtendedStyles();
        }

        return ex_style;
    }

    DWORD NativeControl::GetAdditionalRTLStyle() const
    {
        // If the UI for the view is mirrored, we should make sure we add the
        // extended window style for a right-to-left layout so the subclass creates
        // a mirrored HWND for the underlying control.
        DWORD ex_style = 0;
        if(base::i18n::IsRTL())
        {
            ex_style |= ui::GetExtendedTooltipStyles();
        }

        return ex_style;
    }

    // static
    LRESULT CALLBACK NativeControl::NativeControlWndProc(HWND window,
        UINT message, WPARAM w_param, LPARAM l_param)
    {
        NativeControl* native_control = static_cast<NativeControl*>(
            ui::ViewProp::GetValue(window, kNativeControlKey));
        DCHECK(native_control);
        WNDPROC original_handler = native_control->container_->original_handler_;
        DCHECK(original_handler);

        if(message==WM_KEYDOWN &&
            native_control->OnKeyDown(ui::KeyboardCodeForWindowsKeyCode(w_param)))
        {
            return 0;
        }
        else if(message == WM_SETFOCUS)
        {
            // Let the focus manager know that the focus changed.
            FocusManager* focus_manager = native_control->GetFocusManager();
            if(focus_manager)
            {
                focus_manager->SetFocusedView(native_control);
            }
            else
            {
                NOTREACHED();
            }
        }
        else if(message == WM_DESTROY)
        {
            ui::SetWindowProc(window, reinterpret_cast<WNDPROC>(original_handler));
            native_control->container_->prop_.reset();
        }

        return CallWindowProc(reinterpret_cast<WNDPROC>(original_handler), window,
            message, w_param, l_param);
    }

} //namespace view