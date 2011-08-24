
#ifndef __demo_textfield_h__
#define __demo_textfield_h__

#pragma once

#include "view/controls/textfield/textfield_controller.h"

#include "demo_base.h"

namespace view
{
    class RichView;
    class Textfield;
}

class DemoTextfiled : public DemoBase, public view::TextfieldController
{
public:
    explicit DemoTextfiled(DemoMain* main);
    virtual ~DemoTextfiled();

    // Overridden from DemoBase:
    virtual std::wstring GetDemoTitle();
    virtual void CreateDemoView(view::View* container);

    // Overridden from view::TextfieldController:
    virtual void ContentsChanged(view::Textfield* sender,
        const string16& new_contents);
    virtual bool HandleKeyEvent(view::Textfield* sender,
        const view::KeyEvent& key_event);

private:
    view::Textfield* name_;
    view::Textfield* password_;
    view::Textfield* multi_line_;
    view::RichView* richview_;

    DISALLOW_COPY_AND_ASSIGN(DemoTextfiled);
};

#endif //__demo_textfield_h__