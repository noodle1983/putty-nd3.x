
#ifndef __base_stl_utilinl_h__
#define __base_stl_utilinl_h__

#pragma once

#include <set>
#include <string>
#include <vector>

// 清理STL对象占用的内存.
// STL的clear()/reserve(0)并不一定会释放内存.
// 函数通过swap确保内存释放.
template<class T>
void STLClearObject(T* obj)
{
    T tmp;
    tmp.swap(*obj);
    obj->reserve(0); // 因为"T tmp"有时候会分配内存.
                     // 使用reserve()清理, 不起作用也不要紧.
}

// 如果对象占用空间大于"limit"字节则清理内存. 缺省值是1MB.
template<class T>
inline void STLClearIfBig(T* obj, size_t limit = 1<<20)
{
    if(obj->capacity() >= limit)
    {
        STLClearObject(obj);
    }
    else
    {
        obj->clear();
    }
}

// 为STL对象预留空间.
// STL的reserve()会进行拷贝操作.
// 这里如果有足够容量则不进行拷贝.
template<class T>
void STLReserveIfNeeded(T* obj, int new_size)
{
    if(obj->capacity() < new_size) // 增加容量.
    {
        obj->reserve(new_size);
    }
    else if(obj->size() > new_size) // 减少实际大小.
    {
        obj->resize(new_size);
    }
}

// STLDeleteContainerPointers()
// 对容器区间的指针调用delete(非数组版本)操作.
// 注意: 本可以实现一个DeleteObject仿函数, 调用for_each()进行操作. 这样需要
// 引入algorithm.h, 代价较高.
// 对于hash_[multi]set集合, 要注意在迭代器递增后再进行删除操作, 因为hash_set
// 在迭代器递增的时候可能会对迭代器调用哈希函数, 导致访问野指针.
template<class ForwardIterator>
void STLDeleteContainerPointers(ForwardIterator begin, ForwardIterator end)
{
    while(begin != end)
    {
        ForwardIterator temp = begin;
        ++begin;
        delete *temp;
    }
}

// STLDeleteContainerPairPointers()
// 对容器区间的pairs的key和value调用delete(非数组版本)操作.
// 注意: 和STLDeleteContainerPointers一样, 必须在迭代器递增后进行删除操作.
// 容器在迭代器递增的时候可能会对迭代器调用哈希函数, 导致访问野指针.
template<class ForwardIterator>
void STLDeleteContainerPairPointers(ForwardIterator begin,
                                    ForwardIterator end)
{
    while(begin != end)
    {
        ForwardIterator temp = begin;
        ++begin;
        delete temp->first;
        delete temp->second;
    }
}

// STLDeleteContainerPairFirstPointers()
// 对容器区间的pairs的key调用delete(非数组版本)操作.
// 注意: 和STLDeleteContainerPointers一样, 必须在迭代器递增后进行删除操作.
template<class ForwardIterator>
void STLDeleteContainerPairFirstPointers(ForwardIterator begin,
                                         ForwardIterator end)
{
    while(begin != end)
    {
        ForwardIterator temp = begin;
        ++begin;
        delete temp->first;
    }
}

// STLDeleteContainerPairSecondPointers()
// 对容器区间的pairs的value调用delete(非数组版本)操作.
template<class ForwardIterator>
void STLDeleteContainerPairSecondPointers(ForwardIterator begin,
                                          ForwardIterator end)
{
    while(begin != end)
    {
        delete begin->second;
        ++begin;
    }
}

template<typename T>
inline void STLAssignToVector(std::vector<T>* vec,
                              const T* ptr,
                              size_t n)
{
    vec->resize(n);
    memcpy(&vec->front(), ptr, n*sizeof(T));
}

/***** 快速给vector赋值的Hack方式 *****/

// 这种做法在给vector赋值32字节时, 时钟循环耗费从250降低到大约140.
// 用法:
//     STLAssignToVectorChar(&vec, ptr, size);
//     STLAssignToString(&str, ptr, size);

inline void STLAssignToVectorChar(std::vector<char>* vec,
                                  const char* ptr,
                                  size_t n)
{
    STLAssignToVector(vec, ptr, n);
}

inline void STLAssignToString(std::string* str, const char* ptr, size_t n)
{
    str->resize(n);
    memcpy(&*str->begin(), ptr, n);
}

// 把一个可能为空的vector看作普通数组.
// 如果v不可能为空, 可直接使用&*v.begin(), 但当v为空的时候内存可能有效.
// 这里的函数代码是最有效的, 综合考虑了STL的实际实现. 代码是不可移植的,
// 因此在不可移植的代码中调用. 如果STL实现改变, 这里也需要修改.

template<typename T>
inline T* vector_as_array(std::vector<T>* v)
{
# ifdef NDEBUG
    return &*v->begin();
# else
    return v->empty() ? NULL : &*v->begin();
# endif
}

template<typename T>
inline const T* vector_as_array(const std::vector<T>* v)
{
# ifdef NDEBUG
    return &*v->begin();
# else
    return v->empty() ? NULL : &*v->begin();
# endif
}

// 返回一个指向string内部缓冲区的char*, 可能不是null结尾.
// 对指针写操作会修改string.
//
// 当0<=i<str.size()时, string_as_array(&str)[i]一直有效直到下一次调用
// string方法使迭代器非法.
//
// 自2006-04起, 没有一个标准的方法获取string内部缓冲区的mutable引用.
// 但issue 530
// (http://www.open-std.org/JTC1/SC22/WG21/docs/lwg-active.html#530)
// 建议使用这种方法. 据Matt Austern说, 这种方法在当前所有实现下正常.
inline char* string_as_array(std::string* str)
{
    // 千万不要使用const_cast<char*>(str->data())!
    return str->empty() ? NULL : &*str->begin();
}

// 测试两个hash maps/sets是否等价. STL的==操作在maps/sets元素一致的时候
// 返回false, 因为内部的hash表可以因为插入和删除的顺序不同而有差异.

template<class HashSet>
inline bool HashSetEquality(const HashSet& set_a, const HashSet& set_b)
{
    if(set_a.size() != set_b.size()) return false;
    for(typename HashSet::const_iterator i=set_a.begin();
        i!=set_a.end(); ++i)
    {
        if(set_b.find(*i) == set_b.end())
        {
            return false;
        }
    }
    return true;
}

template<class HashMap>
inline bool HashMapEquality(const HashMap& map_a, const HashMap& map_b)
{
    if(map_a.size() != map_b.size()) return false;
    for(typename HashMap::const_iterator i=map_a.begin();
        i!=map_a.end(); ++i)
    {
        typename HashMap::const_iterator j = map_b.find(i->first);
        if(j == map_b.end()) return false;
        if(i->second != j->second) return false;
    }
    return true;
}

// 以下函数用于清理元素指向分配内存的STL容器.

// STLDeleteElements()删除STL容器中的所有元素并清理容器. 函数适合用于vector、set、
// hash_set以及任何定义begin()、end()和clear()方法的STL容器.
//
// 如果容器为空, 函数无任何操作.
//
// 还有一种不直接调用STLDeleteElements()的方法, 用STLElementDeleter(定义在下面)
// 能确保离开作用域的时候删除容器元素.
template<class T>
void STLDeleteElements(T* container)
{
    if(!container) return;
    STLDeleteContainerPointers(container->begin(), container->end());
    container->clear();
}

// STLDeleteValues删除元素为(key, value)容器中所有的"value"并清理容器.
// NULL指针不做任何操作.
template<class T>
void STLDeleteValues(T* v)
{
    if(!v) return;
    for(typename T::iterator i=v->begin(); i!=v->end(); ++i)
    {
        delete i->second;
    }
    v->clear();
}

// 下面的类提供了一种方便的手段, 当离开作用域时删除STL容器中所有元素
// 或者"value". 这大大的简化了临时对象的创建以及多处返回. 例如:
//     vector<MyProto*> tmp_proto;
//     STLElementDeleter<vector<MyProto*> > d(&tmp_proto);
//     if(...) return false;
//     ...
//     return success;

// 给定STL容器指针, 离开作用域的时候删除所有元素.
template<class STLContainer>
class STLElementDeleter
{
public:
    STLElementDeleter<STLContainer>(STLContainer* ptr) : container_ptr_(ptr) {}
    ~STLElementDeleter<STLContainer>() { STLDeleteElements(container_ptr_); }
private:
    STLContainer* container_ptr_;
};

// 给定STL容器指针, 离开作用域的时候删除所有"value".
template<class STLContainer>
class STLValueDeleter
{
public:
    STLValueDeleter<STLContainer>(STLContainer* ptr) : container_ptr_(ptr) {}
    ~STLValueDeleter<STLContainer>() { STLDeleteValues(container_ptr_); }
private:
    STLContainer* container_ptr_;
};

// Forward declare some callback classes in callback.h for STLBinaryFunction
template<class R, class T1, class T2>
class ResultCallback2;

// STLBinaryFunction is a wrapper for the ResultCallback2 class in callback.h
// It provides an operator () method instead of a Run method, so it may be
// passed to STL functions in <algorithm>.
//
// The client should create callback with NewPermanentCallback, and should
// delete callback after it is done using the STLBinaryFunction.

template<class Result, class Arg1, class Arg2>
class STLBinaryFunction : public std::binary_function<Arg1, Arg2, Result>
{
public:
    typedef ResultCallback2<Result, Arg1, Arg2> Callback;

    STLBinaryFunction(Callback* callback)
        : callback_(callback)
    {
            assert(callback_);
    }

    Result operator() (Arg1 arg1, Arg2 arg2)
    {
        return callback_->Run(arg1, arg2);
    }

private:
    Callback* callback_;
};

// STLBinaryPredicate is a specialized version of STLBinaryFunction, where the
// return type is bool and both arguments have type Arg.  It can be used
// wherever STL requires a StrictWeakOrdering, such as in sort() or
// lower_bound().
//
// templated typedefs are not supported, so instead we use inheritance.

template<class Arg>
class STLBinaryPredicate : public STLBinaryFunction<bool, Arg, Arg>
{
public:
    typedef typename STLBinaryPredicate<Arg>::Callback Callback;
    STLBinaryPredicate(Callback* callback)
        : STLBinaryFunction<bool, Arg, Arg>(callback) {}
};

// Functors that compose arbitrary unary and binary functions with a
// function that "projects" one of the members of a pair.
// Specifically, if p1 and p2, respectively, are the functions that
// map a pair to its first and second, respectively, members, the
// table below summarizes the functions that can be constructed:
//
// * UnaryOperate1st<pair>(f) returns the function x -> f(p1(x))
// * UnaryOperate2nd<pair>(f) returns the function x -> f(p2(x))
// * BinaryOperate1st<pair>(f) returns the function (x,y) -> f(p1(x),p1(y))
// * BinaryOperate2nd<pair>(f) returns the function (x,y) -> f(p2(x),p2(y))
//
// A typical usage for these functions would be when iterating over
// the contents of an STL map. For other sample usage, see the unittest.

template<typename Pair, typename UnaryOp>
class UnaryOperateOnFirst
    : public std::unary_function<Pair, typename UnaryOp::result_type>
{
public:
    UnaryOperateOnFirst() {}

    UnaryOperateOnFirst(const UnaryOp& f) : f_(f) {}

    typename UnaryOp::result_type operator()(const Pair& p) const
    {
        return f_(p.first);
    }

private:
    UnaryOp f_;
};

template<typename Pair, typename UnaryOp>
UnaryOperateOnFirst<Pair, UnaryOp> UnaryOperate1st(const UnaryOp& f)
{
    return UnaryOperateOnFirst<Pair, UnaryOp>(f);
}

template<typename Pair, typename UnaryOp>
class UnaryOperateOnSecond
    : public std::unary_function<Pair, typename UnaryOp::result_type>
{
public:
    UnaryOperateOnSecond() {}

    UnaryOperateOnSecond(const UnaryOp& f) : f_(f) {}

    typename UnaryOp::result_type operator()(const Pair& p) const
    {
        return f_(p.second);
    }

private:
    UnaryOp f_;
};

template<typename Pair, typename UnaryOp>
UnaryOperateOnSecond<Pair, UnaryOp> UnaryOperate2nd(const UnaryOp& f)
{
    return UnaryOperateOnSecond<Pair, UnaryOp>(f);
}

template<typename Pair, typename BinaryOp>
class BinaryOperateOnFirst
    : public std::binary_function<Pair, Pair, typename BinaryOp::result_type>
{
public:
    BinaryOperateOnFirst() {}

    BinaryOperateOnFirst(const BinaryOp& f) : f_(f) {}

    typename BinaryOp::result_type operator()(const Pair& p1,
        const Pair& p2) const
    {
        return f_(p1.first, p2.first);
    }

private:
    BinaryOp f_;
};

template<typename Pair, typename BinaryOp>
BinaryOperateOnFirst<Pair, BinaryOp> BinaryOperate1st(const BinaryOp& f)
{
    return BinaryOperateOnFirst<Pair, BinaryOp>(f);
}

template<typename Pair, typename BinaryOp>
class BinaryOperateOnSecond
    : public std::binary_function<Pair, Pair, typename BinaryOp::result_type>
{
public:
    BinaryOperateOnSecond() {}

    BinaryOperateOnSecond(const BinaryOp& f) : f_(f) {}

    typename BinaryOp::result_type operator()(const Pair& p1,
        const Pair& p2) const
    {
        return f_(p1.second, p2.second);
    }

private:
    BinaryOp f_;
};

template<typename Pair, typename BinaryOp>
BinaryOperateOnSecond<Pair, BinaryOp> BinaryOperate2nd(const BinaryOp& f)
{
    return BinaryOperateOnSecond<Pair, BinaryOp>(f);
}

// 转换set到vector.
template<typename T>
std::vector<T> SetToVector(const std::set<T>& values)
{
    std::vector<T> result;
    result.reserve(values.size());
    result.insert(result.begin(), values.begin(), values.end());
    return result;
}

// 测试set、map、hash_set或者hash_map是否含有指定key. 存在返回true.
template<typename Collection, typename Key>
bool ContainsKey(const Collection& collection, const Key& key)
{
    return collection.find(key) != collection.end();
}

#endif //__base_stl_utilinl_h__