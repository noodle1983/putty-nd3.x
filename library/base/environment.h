
#ifndef __base_environment_h__
#define __base_environment_h__

#pragma once

#include <string>

namespace base
{

    class Environment
    {
    public:
        virtual ~Environment();

        // 静态工厂方法返回特定平台实现的实例.
        static Environment* Create();

        // 获取环境变量的值存于|result|. 如果key未设置返回false.
        virtual bool GetVar(const char* variable_name, std::string* result) = 0;

        // 等同GetVar(variable_name, NULL);
        virtual bool HasVar(const char* variable_name);

        // 成功返回true, 否则返回false.
        virtual bool SetVar(const char* variable_name,
            const std::string& new_value) = 0;

        // 成功返回true, 否则返回false.
        virtual bool UnSetVar(const char* variable_name) = 0;
    };

} //namespace base

#endif //__base_environment_h__