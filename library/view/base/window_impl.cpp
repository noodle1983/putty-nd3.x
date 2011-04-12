
#include "window_impl.h"

#include <list>

#include "base/memory/singleton.h"
#include "base/string_number_conversions.h"
#include "base/win/win_util.h"
#include "base/win/wrapped_window_proc.h"

#include "../base/hwnd_util.h"

namespace view
{

    static const DWORD kWindowDefaultChildStyle =
        WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS;
    static const DWORD kWindowDefaultStyle = WS_OVERLAPPEDWINDOW;
    static const DWORD kWindowDefaultExStyle = 0;

    ///////////////////////////////////////////////////////////////////////////////
    // 维护WindowImpl类.

    // 有几个外部的脚本显式依赖这个类名前缀, 用于获取窗口句柄. 如果它被修改, 将获取
    // 窗口句柄的功能将会失效.
    const wchar_t* const WindowImpl::kBaseClassName = L"Wan_WidgetWin_";

    // WindowImpl的类信息, 用于注册唯一的窗口类.
    struct ClassInfo
    {
        UINT style;
        HBRUSH background;

        explicit ClassInfo(int style) : style(style), background(NULL) {}

        // 比较两个ClassInfos. 如果所有成员匹配返回true.
        bool Equals(const ClassInfo& other) const
        {
            return (other.style==style && other.background==background);
        }
    };

    class ClassRegistrar
    {
    public:
        static ClassRegistrar* GetInstance()
        {
            return base::Singleton<ClassRegistrar>::get();
        }

        ~ClassRegistrar()
        {
            for(RegisteredClasses::iterator i=registered_classes_.begin();
                i!=registered_classes_.end(); ++i)
            {
                UnregisterClass(i->name.c_str(), NULL);
            }
        }

        // 把与|class_info|匹配的注册类的类名设置到|class_name|, 如果没有匹配的则创建
        // 一个新的类名.
        // 如果有匹配的注册类返回true, 否则返回false.
        bool RetrieveClassName(const ClassInfo& class_info, std::wstring* name)
        {
            for(RegisteredClasses::const_iterator i=registered_classes_.begin();
                i!=registered_classes_.end(); ++i)
            {
                if(class_info.Equals(i->info))
                {
                    name->assign(i->name);
                    return true;
                }
            }

            name->assign(string16(WindowImpl::kBaseClassName) +
                base::IntToString16(registered_count_++));
            return false;
        }

        void RegisterClass(const ClassInfo& class_info,
            const std::wstring& name, ATOM atom)
        {
            registered_classes_.push_back(RegisteredClass(class_info, name, atom));
        }

    private:
        // 表示一个注册的窗口类.
        struct RegisteredClass
        {
            RegisteredClass(const ClassInfo& info, const std::wstring& name,
                ATOM atom) : info(info), name(name), atom(atom) {}

            // 用于创建窗口类的信息.
            ClassInfo info;

            // 窗口类名.
            std::wstring name;

            // 注册窗口类返回的ATOM.
            ATOM atom;
        };

        ClassRegistrar() : registered_count_(0) {}
        friend struct base::DefaultSingletonTraits<ClassRegistrar>;

        typedef std::list<RegisteredClass> RegisteredClasses;
        RegisteredClasses registered_classes_;

        // 当前已经注册的窗口类数量.
        int registered_count_;

        DISALLOW_COPY_AND_ASSIGN(ClassRegistrar);
    };


    WindowImpl::WindowImpl()
        : window_style_(0),
        window_ex_style_(kWindowDefaultExStyle),
        class_style_(CS_DBLCLKS),
        hwnd_(NULL) {}

    WindowImpl::~WindowImpl() {}

    void WindowImpl::Init(HWND parent, const gfx::Rect& bounds)
    {
        if(window_style_ == 0)
        {
            window_style_ = parent ? kWindowDefaultChildStyle : kWindowDefaultStyle;
        }

        // 确保传入的父窗口是合法的, 否则CreateWindowEx会失败.
        if(parent && !::IsWindow(parent))
        {
            NOTREACHED() << "invalid parent window specified.";
            parent = NULL;
        }

        int x, y, width, height;
        if(bounds.IsEmpty())
        {
            x = y = width = height = CW_USEDEFAULT;
        }
        else
        {
            x = bounds.x();
            y = bounds.y();
            width = bounds.width();
            height = bounds.height();
        }

        hwnd_ = CreateWindowEx(window_ex_style_, GetWindowClassName().c_str(), NULL,
            window_style_, x, y, width, height, parent, NULL, NULL, this);
        DCHECK(hwnd_);

        // 在窗口过程中应该已经设置过窗口用户数据为this.
        DCHECK(GetWindowUserData(hwnd_) == this);
    }

    HICON WindowImpl::GetDefaultWindowIcon() const
    {
        return NULL;
    }

    // static
    bool WindowImpl::IsWindowImpl(HWND hwnd)
    {
        wchar_t tmp[128];
        if(!::GetClassName(hwnd, tmp, 128))
        {
            return false;
        }

        std::wstring class_name(tmp);
        return class_name.find(kBaseClassName) == 0;
    }

    LRESULT WindowImpl::OnWndProc(UINT message, WPARAM w_param, LPARAM l_param)
    {
        LRESULT result = 0;

        // 处理消息映射表中的消息; 否交由系统处理.
        if(!ProcessWindowMessage(hwnd_, message, w_param, l_param, result))
        {
            result = DefWindowProc(hwnd_, message, w_param, l_param);
        }

        return result;
    }

    // static
    LRESULT CALLBACK WindowImpl::WndProc(HWND hwnd,
        UINT message, WPARAM w_param, LPARAM l_param)
    {
        if(message == WM_NCCREATE)
        {
            CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(l_param);
            WindowImpl* window = reinterpret_cast<WindowImpl*>(cs->lpCreateParams);
            DCHECK(window);
            SetWindowUserData(hwnd, window);
            window->hwnd_ = hwnd;
            return TRUE;
        }

        WindowImpl* window = reinterpret_cast<WindowImpl*>(
            GetWindowUserData(hwnd));
        if(!window)
        {
            return 0;
        }

        return window->OnWndProc(message, w_param, l_param);
    }

    std::wstring WindowImpl::GetWindowClassName()
    {
        ClassInfo class_info(initial_class_style());
        std::wstring name;
        if(ClassRegistrar::GetInstance()->RetrieveClassName(class_info, &name))
        {
            return name;
        }

        // 没找到, 需要注册一个.
        WNDCLASSEX class_ex;
        class_ex.cbSize = sizeof(WNDCLASSEX);
        class_ex.style = class_info.style;
        class_ex.lpfnWndProc = base::WrappedWindowProc<&WindowImpl::WndProc>;
        class_ex.cbClsExtra = 0;
        class_ex.cbWndExtra = 0;
        class_ex.hInstance = NULL;
        class_ex.hIcon = GetDefaultWindowIcon();
        class_ex.hCursor = LoadCursor(NULL, IDC_ARROW);
        class_ex.hbrBackground = reinterpret_cast<HBRUSH>(class_info.background+1);
        class_ex.lpszMenuName = NULL;
        class_ex.lpszClassName = name.c_str();
        class_ex.hIconSm = class_ex.hIcon;
        ATOM atom = RegisterClassEx(&class_ex);
        DCHECK(atom);

        ClassRegistrar::GetInstance()->RegisterClass(class_info, name, atom);

        return name;
    }

} //namespace view