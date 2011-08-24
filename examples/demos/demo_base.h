
#ifndef __demo_base_h__
#define __demo_base_h__

#pragma once

#include <string>

#include "base/basic_types.h"

namespace view
{
    class View;
}

class DemoMain;

class DemoBase
{
public:
    view::View* GetDemoView() { return container_; }

    virtual std::wstring GetDemoTitle() = 0;
    virtual void CreateDemoView(view::View* parent) = 0;

protected:
    explicit DemoBase(DemoMain* main);
    virtual ~DemoBase() {}

    void PrintStatus(const std::wstring& status);

    DemoMain* demo_main() const { return main_; }

private:
    DemoMain* main_;

    view::View* container_;

    DISALLOW_COPY_AND_ASSIGN(DemoBase);
};

#endif //__demo_base_h__