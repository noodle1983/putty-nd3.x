
#ifndef __chrome_main_h__
#define __chrome_main_h__

#pragma once

#include "base/basic_types.h"

class ChromeMain
{
public:
    ChromeMain();
    virtual ~ChromeMain();

    void Run();

private:
    DISALLOW_COPY_AND_ASSIGN(ChromeMain);
};

#endif //__chrome_main_h__