
#ifndef __view_accessible_view_state_h__
#define __view_accessible_view_state_h__

#pragma once

#include "base/string16.h"

#include "accessibility_types.h"

////////////////////////////////////////////////////////////////////////////////
//
// AccessibleViewState
//
//   存储可访问信息的跨平台结构体, 为UI视图提供辅助技术(AT).
//
////////////////////////////////////////////////////////////////////////////////
struct AccessibleViewState
{
public:
    AccessibleViewState();
    ~AccessibleViewState();

    // 视图的角色, 像button或者list box.
    AccessibilityTypes::Role role;

    // 视图的状态, 是选中(复选框)和保护(密码框)这类字段的位与.
    AccessibilityTypes::State state;

    // 视图的名字/标签.
    string16 name;

    // 视图的值, 比如文本内容.
    string16 value;

    // 用户点击视图的缺省动作名.
    string16 default_action;

    // 激活视图的键盘快捷键.
    string16 keyboard_shortcut;

    // 选中的起始结束位置. 只对带有文本内容的视图有效, 比如文本框或者组合框;
    // 其它的起始和结束都会-1.
    int selection_start;
    int selection_end;

    // The selected item's index and the count of the number of items.
    // Only applies to views with multiple choices like a listbox; both
    // index and count should be -1 otherwise.
    // 选中条目的索引和总条目数. 只对多选视图有效, 比如列表; 其它的视图
    // 均为-1.
    int index;
    int count;
};

#endif //__view_accessible_view_state_h__