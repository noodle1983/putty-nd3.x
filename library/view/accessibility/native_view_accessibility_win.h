
#ifndef __view_native_view_accessibility_win_h__
#define __view_native_view_accessibility_win_h__

#pragma once

#include <atlbase.h>
#include <atlcom.h>

#include <oleacc.h>

#include "base/ref_counted.h"

#include "../controls/native/native_view_host.h"
#include "../view/view.h"
#include "accessible_view_state.h"

namespace view
{
    extern const char kViewNativeHostPropForAccessibility[];
    // 注意: 不要把NativeViewAccessibilityWin放在名字空间"view"中;
    // Visual Studio 2005不允许ATL::CComObject符号在名字空间中.
}

////////////////////////////////////////////////////////////////////////////////
//
// NativeViewAccessibilityWin
//
// 为View实现MSAA IAccessible COM接口, 为提供屏幕阅读者或者其它辅助技术(AT)提供
// 访问性支持.
//
////////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE NativeViewAccessibilityWin
    : public CComObjectRootEx<CComMultiThreadModel>,
    public IDispatchImpl<IAccessible, &IID_IAccessible, &LIBID_Accessibility>
{
public:
    BEGIN_COM_MAP(NativeViewAccessibilityWin)
        COM_INTERFACE_ENTRY2(IDispatch, IAccessible)
        COM_INTERFACE_ENTRY(IAccessible)
    END_COM_MAP()

    // 视图可访问性创建方法.
    static scoped_refptr<NativeViewAccessibilityWin> Create(view::View* view);

    // 返回视图的IAccessible接口.
    static IAccessible* GetAccessibleForView(view::View* view);

    virtual ~NativeViewAccessibilityWin();

    void set_view(view::View* view) { view_ = view; }

    // 支持的IAccessible方法.

    // 返回屏幕上给定点的子元素或者子对象.
    STDMETHODIMP accHitTest(LONG x_left, LONG y_top, VARIANT* child);

    // 执行对象的缺省动作.
    STDMETHODIMP accDoDefaultAction(VARIANT var_id);

    // 返回指定对象的当前屏幕位置.
    STDMETHODIMP accLocation(LONG* x_left,
        LONG* y_top,
        LONG* width,
        LONG* height,
        VARIANT var_id);

    // 遍历到其它UI元素并返回.
    STDMETHODIMP accNavigate(LONG nav_dir, VARIANT start, VARIANT* end);

    // 返回指定孩子的IDispatch接口指针.
    STDMETHODIMP get_accChild(VARIANT var_child, IDispatch** disp_child);

    // 返回可访问的孩子数目.
    STDMETHODIMP get_accChildCount(LONG* child_count);

    // 返回对象缺省动作的字符串描述.
    STDMETHODIMP get_accDefaultAction(VARIANT var_id, BSTR* default_action);

    // 返回提示信息.
    STDMETHODIMP get_accDescription(VARIANT var_id, BSTR* desc);

    // 返回对象是否有键盘焦点.
    STDMETHODIMP get_accFocus(VARIANT* focus_child);

    // 返回指定对象的快捷键.
    STDMETHODIMP get_accKeyboardShortcut(VARIANT var_id, BSTR* access_key);

    // 返回指定对象的名字.
    STDMETHODIMP get_accName(VARIANT var_id, BSTR* name);

    // 返回对象父亲的IDispatch接口指针.
    STDMETHODIMP get_accParent(IDispatch** disp_parent);

    // 返回指定对象的角色描述信息.
    STDMETHODIMP get_accRole(VARIANT var_id, VARIANT* role);

    // 返回指定对象的当前状态.
    STDMETHODIMP get_accState(VARIANT var_id, VARIANT* state);

    // 返回指定对象的当前值.
    STDMETHODIMP get_accValue(VARIANT var_id, BSTR* value);

    // 不支持的IAccessible方法.

    // 视图的选择是不可应用的.
    STDMETHODIMP get_accSelection(VARIANT* selected);
    STDMETHODIMP accSelect(LONG flags_sel, VARIANT var_id);

    // 不支持帮助功能.
    STDMETHODIMP get_accHelp(VARIANT var_id, BSTR* help);
    STDMETHODIMP get_accHelpTopic(BSTR* help_file,
        VARIANT var_id, LONG* topic_id);

    // 废弃的功能, 这里不实现.
    STDMETHODIMP put_accName(VARIANT var_id, BSTR put_name);
    STDMETHODIMP put_accValue(VARIANT var_id, BSTR put_val);

    // 事件(accessibility_types.h中定义的)转换到MSAA事件, 并返回.
    static int32 MSAAEvent(AccessibilityTypes::Event event);

    // 角色(accessibility_types.h中定义的)转换到MSAA角色, 并返回.
    static int32 MSAARole(AccessibilityTypes::Role role);

    // 状态(accessibility_types.h中定义的)转换到MSAA状态, 并返回.
    static int32 MSAAState(AccessibilityTypes::State state);

private:
    NativeViewAccessibilityWin();

    // 判断accNavigate的导航方向, 左、上和前映射为前, 右、下和后映射为后.
    // 如果导航方向是后返回true, 否则返回false.
    bool IsNavDirNext(int nav_dir) const;

    // 判断导航目标是否在允许范围. 如果是则返回true, 否则返回false.
    bool IsValidNav(int nav_dir,
        int start_id,
        int lower_bound,
        int upper_bound) const;

    // 判断child是否合法.
    bool IsValidId(const VARIANT& child) const;

    // 设置视图可应用状态的辅助函数.
    void SetState(VARIANT* msaa_state, view::View* view);

    // 返回本地视图的IAccessible接口(如果是可应用的). 成功返回S_OK.
    static HRESULT GetNativeIAccessibleInterface(
        view::NativeViewHost* native_host,
        IAccessible** accessible);

    static HRESULT GetNativeIAccessibleInterface(HWND native_view_window,
        IAccessible** accessible);

    // 允许CComObject访问类构造函数.
    template<class Base> friend class CComObject;

    // 视图成员.
    view::View* view_;

    DISALLOW_COPY_AND_ASSIGN(NativeViewAccessibilityWin);
};

#endif //__view_native_view_accessibility_win_h__