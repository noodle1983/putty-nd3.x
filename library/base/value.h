
// 定义递归的数据存储类Value, 用于存储设置以及其它持续化数据. 可以包含(递归的)
// lists和dictionaries, 所以非常实用. 然而, API按照常见的方式进行了优化, 也就
// 是说存储了一个简单数据的树状结构. 给定DictionaryValue根, 可以方便的做到:
//
// root->SetString("global.pages.homepage", "http://goateleporter.com");
// std::string homepage = "http://google.com"; // default/fallback value
// root->GetString("global.pages.homepage", &homepage);
//
// "global"和"pages"是DictionaryValues, "homepage"是字符串. 如果某些路径上的
// 元素不存在, SetString()方法在添加数据前会创建丢失的元素并附加到根上.

#ifndef __base_value_h__
#define __base_value_h__

#pragma once

#include <map>
#include <string>
#include <vector>

#include "basic_types.h"
#include "string16.h"

namespace base
{

    class BinaryValue;
    class DictionaryValue;
    class FundamentalValue;
    class ListValue;
    class StringValue;
    class Value;

    typedef std::vector<Value*> ValueVector;
    typedef std::map<std::string, Value*> ValueMap;

    // Value类是其它Values类的基类. Value可以通过Create*Value()工厂方法
    // 实例化, 或者直接由子类实例化.
    class Value
    {
    public:
        enum Type
        {
            TYPE_NULL = 0,
            TYPE_BOOLEAN,
            TYPE_INTEGER,
            TYPE_DOUBLE,
            TYPE_STRING,
            TYPE_BINARY,
            TYPE_DICTIONARY,
            TYPE_LIST
        };

        virtual ~Value();

        // 创建各种Value对象方便的接口, 无须知道具体类的实现.
        // 总是可以返回一个合法的Value*.
        static Value* CreateNullValue();
        static FundamentalValue* CreateBooleanValue(bool in_value);
        static FundamentalValue* CreateIntegerValue(int in_value);
        static FundamentalValue* CreateDoubleValue(double in_value);
        static StringValue* CreateStringValue(const std::string& in_value);
        static StringValue* CreateStringValue(const string16& in_value);

        // 返回当前Value对象存储的数据类型. 每种类型只会有一个子类, 因此通过
        // Type确定并向下类型转换Value*到(Implementing Class)*是安全的.
        // 同时, Value对象构建之后不会修改类型.
        Type GetType() const { return type_; }

        // 如果当前对象是指定类型则返回true.
        bool IsType(Type type) const { return type == type_; }

        // 以下方法方便的获取设置. 如果当前对象能转换到指定类型, 通过|out_value|
        // 参数返回数据, 函数返回true. 否则函数返回false, |out_value|未修改.
        virtual bool GetAsBoolean(bool* out_value) const;
        virtual bool GetAsInteger(int* out_value) const;
        virtual bool GetAsDouble(double* out_value) const;
        virtual bool GetAsString(std::string* out_value) const;
        virtual bool GetAsString(string16* out_value) const;
        virtual bool GetAsList(ListValue** out_value);
        virtual bool GetAsList(const ListValue** out_value) const;

        // 创建一份整个树的深拷贝, 返回指针. 调用者拥有拷贝的所有权.
        virtual Value* DeepCopy() const;

        // 比较两个Value对象的内容是否相同.
        virtual bool Equals(const Value* other) const;

        // 比较两个Value对象的内容是否相同. 能处理NULL的情况. 两个NULL被认为相同,
        // 但是跟Value::CreateNullValue()的值不相同.
        static bool Equals(const Value* a, const Value* b);

        // TODO(sky) bug 91396: remove this when we figure out 91396.
        // If true crash when deleted.
        void set_check_on_delete(bool value) { check_on_delete_ = value; }

    protected:
        // 对于调用者来讲是不安全的(应该使用上面的Create*Value()静态方法),
        // 对于子类是有用的.
        explicit Value(Type type);

        // See description above setter.
        bool check_on_delete_;

    private:
        Value();

        Type type_;

        DISALLOW_COPY_AND_ASSIGN(Value);
    };

    // FundamentalValue表示简单的基础数据类型.
    class FundamentalValue : public Value
    {
    public:
        explicit FundamentalValue(bool in_value);
        explicit FundamentalValue(int in_value);
        explicit FundamentalValue(double in_value);
        virtual ~FundamentalValue();

        // Overridden from Value:
        virtual bool GetAsBoolean(bool* out_value) const;
        virtual bool GetAsInteger(int* out_value) const;
        virtual bool GetAsDouble(double* out_value) const;
        virtual FundamentalValue* DeepCopy() const;
        virtual bool Equals(const Value* other) const;

    private:
        union
        {
            bool boolean_value_;
            int integer_value_;
            double double_value_;
        };

        DISALLOW_COPY_AND_ASSIGN(FundamentalValue);
    };

    class StringValue : public Value
    {
    public:
        // 用UTF-8窄字符串初始化一个StringValue.
        explicit StringValue(const std::string& in_value);

        // 用string16初始化一个StringValue.
        explicit StringValue(const string16& in_value);

        virtual ~StringValue();

        // Overridden from Value:
        virtual bool GetAsString(std::string* out_value) const;
        virtual bool GetAsString(string16* out_value) const;
        virtual StringValue* DeepCopy() const;
        virtual bool Equals(const Value* other) const;

    private:
        std::string value_;

        DISALLOW_COPY_AND_ASSIGN(StringValue);
    };

    class BinaryValue: public Value
    {
    public:
        virtual ~BinaryValue();

        // 创建一个表示二进制缓冲区的Value. 如果成功新的对象接管指针的所有权.
        // 如果buffer为NULL返回NULL.
        static BinaryValue* Create(char* buffer, size_t size);

        // 想要维持缓冲区所有权时, 使用该工厂方法拷贝缓冲区内容创建新的BinaryValue.
        // 如果buffer为NULL返回NULL.
        static BinaryValue* CreateWithCopiedBuffer(const char* buffer, size_t size);


        size_t GetSize() const { return size_; }
        char* GetBuffer() { return buffer_; }
        const char* GetBuffer() const { return buffer_; }

        // 基类方法.
        virtual BinaryValue* DeepCopy() const;
        virtual bool Equals(const Value* other) const;

    private:
        // 构造函数私有, 这样只会创建具有合法的缓冲区指针和大小的BinaryValue.
        BinaryValue(char* buffer, size_t size);

        char* buffer_;
        size_t size_;

        DISALLOW_COPY_AND_ASSIGN(BinaryValue);
    };

    // DictionaryValue提供一个key-value字典, 通过"path"解析进行递归访问;
    // 参见文件顶部的注释. Keys是|std::string|s, 需要UTF-8编码.
    class DictionaryValue : public Value
    {
    public:
        DictionaryValue();
        virtual ~DictionaryValue();

        // 当前字典如果有指定的key则返回true.
        bool HasKey(const std::string& key) const;

        // 返回字典的Values数量.
        size_t size() const { return dictionary_.size(); }

        // 返回字典是否为空.
        bool empty() const { return dictionary_.empty(); }

        // 清除字典所有内容.
        void Clear();

        // 指定路径设置Value. 路径形式是"<key>"或"<key>.<key>.[...]", "."进入字典
        // 的下一层索引. 很显然"."不能出现在key中, 除此没有其它别的限制.
        // 路径上遇到不存在的key, 或者存在但不是DictionaryValue时, 创建一个新的
        // DictionaryValue并附加到路径上对应位置.
        // 注意字典会接管|in_value|的所有权, 所以|in_value|不能为NULL.
        void Set(const std::string& path, Value* in_value);

        // Set()的简化形式. 替换路径上存在的数据, 即使类型不同.
        void SetBoolean(const std::string& path, bool in_value);
        void SetInteger(const std::string& path, int in_value);
        void SetDouble(const std::string& path, double in_value);
        void SetString(const std::string& path, const std::string& in_value);
        void SetString(const std::string& path, const string16& in_value);

        // 和Set()类似, 但对'.'不做特殊处理. 允许将URLs用于路径.
        void SetWithoutPathExpansion(const std::string& key, Value* in_value);

        // 指定路径获取Value. 路径形式是"<key>"或"<key>.<key>.[...]", "."进入字典
        // 的下一层索引. 如果路径解析成功, 通过|out_value|参数返回最后的key对应的
        // 数据, 函数返回true. 否则, 返回false且不修改|out_value|.
        // 注意字典总是拥有返回数据的所有权.
        bool Get(const std::string& path, Value** out_value) const;

        // Get()的简化形式. 如果路径合法且路径末端的值符合指定格式, 则返回数据,
        // 且函数返回true.
        bool GetBoolean(const std::string& path, bool* out_value) const;
        bool GetInteger(const std::string& path, int* out_value) const;
        bool GetDouble(const std::string& path, double* out_value) const;
        bool GetString(const std::string& path, std::string* out_value) const;
        bool GetString(const std::string& path, string16* out_value) const;
        bool GetStringASCII(const std::string& path, std::string* out_value) const;
        bool GetBinary(const std::string& path, BinaryValue** out_value) const;
        bool GetDictionary(const std::string& path,
            DictionaryValue** out_value) const;
        bool GetList(const std::string& path, ListValue** out_value) const;

        // 和Get()类似，但对'.'不做特殊处理. 允许将URLs用于路径.
        bool GetWithoutPathExpansion(const std::string& key,
            Value** out_value) const;
        bool GetIntegerWithoutPathExpansion(const std::string& key,
            int* out_value) const;
        bool GetDoubleWithoutPathExpansion(const std::string& key,
            double* out_value) const;
        bool GetStringWithoutPathExpansion(const std::string& key,
            std::string* out_value) const;
        bool GetStringWithoutPathExpansion(const std::string& key,
            string16* out_value) const;
        bool GetDictionaryWithoutPathExpansion(const std::string& key,
            DictionaryValue** out_value) const;
        bool GetListWithoutPathExpansion(const std::string& key,
            ListValue** out_value) const;

        // 移除字典中指定路径(包括子字典, 如果路径不是一个局部的key)的数据. 如果
        // |out_value|不为空, 移除的数据以及所有权转移到out_value. 如果|out_value|
        // 为空, 删除移除的数据. 如果|path|是合法路径返回true; 否则返回false且
        // DictionaryValue数据不改变.
        bool Remove(const std::string& path, Value** out_value);

        // 类似Remove(), 但对'.'不做特殊处理. 允许将URLs用于路径.
        bool RemoveWithoutPathExpansion(const std::string& key, Value** out_value);

        // 生成一份深拷贝, 但不包括空dictionaries和lists. 不会返回NULL, 即使|this|
        // 为空.
        DictionaryValue* DeepCopyWithoutEmptyChildren();

        // 合并字典. 会递归调用, 比如子字典也会合并. key冲突时, 优先选择传入的字典
        // 数据, 现有的会被替换.
        void MergeDictionary(const DictionaryValue* dictionary);

        // Swaps contents with the |other| dictionary.
        void Swap(DictionaryValue* other)
        {
            dictionary_.swap(other->dictionary_);
        }

        // key_iterator类是字典中的keys的迭代器. 不能用于修改字典.
        //
        // 只能使用XXXWithoutPathExpansion() APIs, 不要使用XXX() APIs. 确保带有
        // '.'的keys能正常工作.
        class key_iterator : private std::iterator<std::input_iterator_tag,
            const std::string>
        {
        public:
            explicit key_iterator(ValueMap::const_iterator itr) { itr_ = itr; }
            key_iterator operator++()
            {
                ++itr_;
                return *this;
            }
            const std::string& operator*() { return itr_->first; }
            bool operator!=(const key_iterator& other) { return itr_ != other.itr_; }
            bool operator==(const key_iterator& other) { return itr_ == other.itr_; }

        private:
            ValueMap::const_iterator itr_;
        };

        key_iterator begin_keys() const { return key_iterator(dictionary_.begin()); }
        key_iterator end_keys() const { return key_iterator(dictionary_.end()); }

        // 基类方法.
        virtual DictionaryValue* DeepCopy() const;
        virtual bool Equals(const Value* other) const;

    private:
        ValueMap dictionary_;

        DISALLOW_COPY_AND_ASSIGN(DictionaryValue);
    };

    // 表示Value数据的列表.
    class ListValue : public Value
    {
    public:
        // 迭代器.
        typedef ValueVector::iterator iterator;
        typedef ValueVector::const_iterator const_iterator;

        ListValue();
        virtual ~ListValue();

        // 清理当前内容.
        void Clear();

        // 返回Values数量.
        size_t GetSize() const { return list_.size(); }

        // 返回是否为空.
        bool empty() const { return list_.empty(); }

        // 设置list指定索引的数据. 如果越界, 填充null数据.
        // 成功返回true, 如果索引为负数或者in_value为空返回false.
        bool Set(size_t index, Value* in_value);

        // 获取指定索引的数据. 当索引在当前列表区间时才修改|out_value|(返回true).
        // 注意list始终拥有数据的所有权.
        bool Get(size_t index, Value** out_value) const;

        // Get()的简化版本. 当索引合法且数据能以指定格式返回时才修改|out_value|
        // (返回true).
        bool GetBoolean(size_t index, bool* out_value) const;
        bool GetInteger(size_t index, int* out_value) const;
        bool GetDouble(size_t index, double* out_value) const;
        bool GetString(size_t index, std::string* out_value) const;
        bool GetString(size_t index, string16* out_value) const;
        bool GetBinary(size_t index, BinaryValue** out_value) const;
        bool GetDictionary(size_t index, DictionaryValue** out_value) const;
        bool GetList(size_t index, ListValue** out_value) const;

        // 从list移除指定索引的数据. 如果|out_value|非空, 移除的数据以及所有权转移到
        // |out_value|. 如果|out_value|为空, 删除移除的数据. 方法只有当|index|合法时
        // 返回true; 否则返回false且ListValue数据不改变.
        bool Remove(size_t index, Value** out_value);

        // 移除查找到的第一个|value|, 并删除, 不存在返回false.
        bool Remove(const Value& value, size_t* index);

        // 在尾部添加一个数据.
        void Append(Value* in_value);

        // 如果数据不存在则添加.
        // 成功返回true, 如果已经存在则返回false.
        bool AppendIfNotPresent(Value* in_value);

        // 在索引处插入数据.
        // 成功返回true, 越界返回false.
        bool Insert(size_t index, Value* in_value);

        // 和|other|互换内容.
        void Swap(ListValue* other)
        {
            list_.swap(other->list_);
        }


        iterator begin() { return list_.begin(); }
        iterator end() { return list_.end(); }

        const_iterator begin() const { return list_.begin(); }
        const_iterator end() const { return list_.end(); }

        // 基类方法.
        virtual bool GetAsList(ListValue** out_value);
        virtual bool GetAsList(const ListValue** out_value) const;
        virtual ListValue* DeepCopy() const;
        virtual bool Equals(const Value* other) const;

    private:
        ValueVector list_;

        DISALLOW_COPY_AND_ASSIGN(ListValue);
    };


    // 接口由知道如何序列化和反序列化Value对象的类实现.
    class ValueSerializer
    {
    public:
        virtual ~ValueSerializer();

        virtual bool Serialize(const Value& root) = 0;

        // 反序列化派生类成Value对象. 如果返回非空, 调用者拥有返回Value所有权. 如果
        // 返回NULL且error_code非空, error_code设置为底层错误码. 如果|error_message|
        // 非空, 填充错误消息最好可能包括错误位置.
        virtual Value* Deserialize(int* error_code, std::string* error_str) = 0;
    };

} //namespace base

#endif //__base_value_h__