
#ifndef __base_scoped_handle_h__
#define __base_scoped_handle_h__

#pragma once

#include <stdio.h>

#include "base/basic_types.h"

class ScopedStdioHandle
{
public:
    ScopedStdioHandle() : handle_(NULL) {}

    explicit ScopedStdioHandle(FILE* handle) : handle_(handle) {}

    ~ScopedStdioHandle()
    {
        Close();
    }

    void Close()
    {
        if(handle_)
        {
            fclose(handle_);
            handle_ = NULL;
        }
    }

    FILE* get() const { return handle_; }

    FILE* Take()
    {
        FILE* temp = handle_;
        handle_ = NULL;
        return temp;
    }

    void Set(FILE* newhandle)
    {
        Close();
        handle_ = newhandle;
    }

private:
    FILE* handle_;

    DISALLOW_COPY_AND_ASSIGN(ScopedStdioHandle);
};

#endif //__base_scoped_handle_h__