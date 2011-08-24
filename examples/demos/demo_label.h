
#ifndef __demo_label_h__
#define __demo_label_h__

#pragma once

#include "view/controls/link_listener.h"
#include "demo_base.h"

namespace view
{
    class Label;
    class Separator;
}

class DemoLabel : public DemoBase, public view::LinkListener
{
public:
    explicit DemoLabel(DemoMain* main);
    virtual ~DemoLabel();

    // Overridden from DemoBase:
    virtual std::wstring GetDemoTitle();
    virtual void CreateDemoView(view::View* container);

    // Overridden from view::LinkListener:
    virtual void LinkClicked(view::Link* source, int event_flags);

private:
    view::Link* link_;
    view::Link* link_custom_;
    view::Link* link_disable_;
    view::Link* link_disable_custom_;

    view::Separator* separator_;

    view::Label* label_;
    view::Label* label_align_left_;
    view::Label* label_align_right_;
    view::Label* label_multi_line_;

    DISALLOW_COPY_AND_ASSIGN(DemoLabel);
};

#endif //__demo_label_h__