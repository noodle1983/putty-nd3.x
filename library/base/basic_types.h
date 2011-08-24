
#ifndef __base_basic_types_h__
#define __base_basic_types_h__

#pragma once

#include <stddef.h>

#include "port.h"

typedef signed char         schar;
typedef signed char         int8;
typedef short               int16;
typedef int                 int32;
#if __LP64__
typedef long                int64;
#else
typedef long long           int64;
#endif
typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
#if __LP64__
typedef unsigned long       uint64;
#else
typedef unsigned long long  uint64;
#endif
typedef signed int          char32;

const uint8  kuint8max  = (( uint8) 0xFF);
const uint16 kuint16max = ((uint16) 0xFFFF);
const uint32 kuint32max = ((uint32) 0xFFFFFFFF);
const uint64 kuint64max = ((uint64) GG_LONGLONG(0xFFFFFFFFFFFFFFFF));
const  int8  kint8min   = ((  int8) 0x80);
const  int8  kint8max   = ((  int8) 0x7F);
const  int16 kint16min  = (( int16) 0x8000);
const  int16 kint16max  = (( int16) 0x7FFF);
const  int32 kint32min  = (( int32) 0x80000000);
const  int32 kint32max  = (( int32) 0x7FFFFFFF);
const  int64 kint64min  = (( int64) GG_LONGLONG(0x8000000000000000));
const  int64 kint64max  = (( int64) GG_LONGLONG(0x7FFFFFFFFFFFFFFF));

// DISALLOW_COPY_AND_ASSIGN禁用拷贝和赋值构造函数.
// 需要在类的private:访问控制域中使用.
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&); \
    void operator=(const TypeName&)

// DISALLOW_IMPLICIT_CONSTRUCTORS禁止隐式的构造函数, 包括缺省构造函数、
// 拷贝构造函数和赋值构造函数.
//
// 需要在类的private:访问控制域中使用以防止实例化, 对于只有静态方法的
// 类非常有用.
#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
    TypeName(); \
    DISALLOW_COPY_AND_ASSIGN(TypeName)

// ArraySizeHelper是一个返回类型为char[N]的函数,其形参类型为 T[N].
// 函数没必要实现, 因为sizeof只需要类型.
template<typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];

// arraysize(arr)返回array数组的元素个数. 该表达式是编译时常量,
// 可以用于定义新的数组. 如果传递一个指针会报编译时错误.
//
// 美中不足的是arraysize(arr)不支持匿名类型和在函数中定义的类型.
// 这种情况下就必须使用非类型安全的ARRAYSIZE_UNSAFE()宏. 这是由
// C++模板机制限制的, 以后会取消.
#define arraysize(array) (sizeof(ArraySizeHelper(array)))

// ARRAYSIZE_UNSAFE所在的工作和arraysize一样, 但可用于匿名类型和
// 在函数中定义的类型. 它没有arraysize安全, 因为可以接受一些指针
// 类型(不是全部), 所以我们应该尽量使用arraysize.
//
// ARRAYSIZE_UNSAFE(a)表达式是类型为size_t的编译时常量.
//
// ARRAYSIZE_UNSAFE可以捕捉一些类型错误. 如果你看到编译错误
//     "warning: division by zero in ..."
// 那是你错误的传递给ARRAYSIZE_UNSAFE了一个指针. ARRAYSIZE_UNSAFE
// 只应该用于静态分配的数组.
//
// ARRAYSIZE_UNSAFE(arr)通过比对sizeof(arr)和sizeof(*(arr))实现的.
// 如果前者被后者可分, arr可能是一个数组, 商就是数组元素个数; 否则
// arr不是数组, 会报编译时错误.
//
// bool类型的大小是具体实现定义的, 所以需要把!(sizeof(a) & sizeof(*(a)))
// 强转成size_t以确保最终结果是size_t.
//
// 这个宏并不完美, 会错误的接受一些指针(指针大小可以整除元素大小).
// 在32位平台上, 指针大小是4字节, 小于3或者大于4字节的指针类型都会报错.
#define ARRAYSIZE_UNSAFE(a) \
    ((sizeof(a)/sizeof(*(a))) / \
    static_cast<size_t>(!(sizeof(a)%sizeof(*(a)))))

// COMPILE_ASSERT宏用来在编译时断言表达式. 例如可以这样保证静态数组大小:
//     COMPILE_ASSERT(ARRAYSIZE_UNSAFE(content_type_names)==CONTENT_NUM_TYPES,
//         content_type_names_incorrect_size);
//
// 或者确保结构体小于一定大小:
//     COMPILE_ASSERT(sizeof(foo)<128, foo_too_large);
// 第二个宏参数是变量名, 如果表达式为false, 编译器会产生一条包含变量名的错误/警告.
template<bool>
struct CompileAssert {};

// COMPILE_ASSERT实现细节:
//
// - COMPILE_ASSERT通过定义一个长度为-1的数组(非法)来实现的, 此时表达式false.
//
// - 下面简化的定义
//       #define COMPILE_ASSERT(expr, msg) typedef char msg[(expr)?1:-1]
//   是非法的. 由于gcc支持运行时确定长度的变长数组(gcc扩展, 不属于C++标准),
//   导致下面这段简单的代码定义不报错:
//       int foo;
//       COMPILE_ASSERT(foo, msg); // not supposed to compile as foo is
//                                 // not a compile-time constant.
//
// - 要使用类型CompileAssert<(bool(expr))>, 必须确保expr是编译时常量.
//   (模板参数在编译时确定.)
//
// - CompileAssert<(bool(expr))>最外层的圆括号用于解决gcc 3.4.4和4.0.1的
//   一个bug. 如果写成
//       CompileAssert<bool(expr)>
//   编译器将无法编译
//       COMPILE_ASSERT(5>0, some_message);
//   ("5>0"中的">"被误认为是模板参数列表结尾的">".)
//
// - 数组大小是(bool(expr)?1:-1)而不是((expr)?1:-1), 可以解决MS VC 7.1
//   中把((0.0)?1:-1)错误计算为1的bug.
#undef COMPILE_ASSERT
#define COMPILE_ASSERT(expr, msg) \
    typedef CompileAssert<(bool(expr))> msg[bool(expr)?1:-1]

// bit_cast<Dest,Source>模板函数实现"*reinterpret_cast<Dest*>(&source)"
// 等同的功能. 在protobuf库和快速算法支持的底层代码中使用.
//
//     float f = 3.14159265358979;
//     int i = bit_cast<int32>(f);
//     // i = 0x40490fdb
//
// 经典的地址强制方法:
//
//     // WRONG
//     float f = 3.14159265358979;            // WRONG
//     int i = * reinterpret_cast<int*>(&f);  // WRONG
//
// 按照ISO C++规范中3.10 -15 -节, 这种做法会产生未定义行为. 概括讲,
// 本节说的是如果程序使用不同的类型访问一个对象的内存地址, 大部分
// 下会导致未定义行为.
//
// 这种说法对于*(int*)&f或者*reinterpret_cast<int*>(&f)都是成立的,
// 尤其是整数左值和浮点数左值进行转换时.
//
// 3.10 -15-目的是为了允许编译器对不同类型的内存引用表达式进行优化.
// gcc 4.0.1利用了这一优化. 所以不规范的程序可能会产生错误的输出.
//
// 问题不在于使用了reinterpret_cast, 而是类型的双关语: 内存中的对象类型
// 和读取字节时的类型不一致.
//
// C++标准是繁缛复杂的.
//
// 然而...
//
// 希望bit_cast<>使用memcpy()能遵守标准, 尤其是3.9节的示例.
// 当然bit_cast<>把不好的逻辑封装集中在一个地方.
//
// memcpy()是非常快的. 在优化模式下, gcc 2.95.3、gcc 4.0.1是常量复杂度,
// msvc 7.1中生成的代码数据搬运最少. 在32位平台, memcpy(d,s,4)进行一次
// 存取操作, memcpy(d,s,8)进行2次存取操作.
//
// 使用gcc 2.95.3、gcc 4.0.1、icc 8.1、and msvc 7.1测试过代码.
//
// 警告: 如果Dest或者Source是非POD类型, memcpy的结果会让你吃惊.
template<class Dest, class Source>
inline Dest bit_cast(const Source& source)
{
    // 编译时断言: sizeof(Dest) == sizeof(Source)
    // 编译错误意味着Dest和Source大小不一致.
    typedef char VerifySizesAreEqual[sizeof(Dest)==sizeof(Source) ? 1 : -1];

    Dest dest;
    memcpy(&dest, &source, sizeof(dest));
    return dest;
}

// LinkerInitialized枚举只应该用做类的构造函数参数, 表明是静态存储类.
// 告诉读者声明一个静态实例是合法的, 传递LINKER_INITIALIZED构造.
// 一般带有构造或者析构函数的类声明静态变量是不安全的, 因为实例化的
// 次序是不确定的. 如果一个类可以用0填充初始化, 且析构函数不操作该内存,
// 类没有虚函数, 它的构造函数可以这样声明:
//     explicit MyClass(base::LinkerInitialized x) {}
// 可以如下调用:
//     static MyClass my_variable_name(base::LINKER_INITIALIZED);
namespace base
{
    enum LinkerInitialized { LINKER_INITIALIZED };
} //namespace base

#endif //__base_basic_types_h__