#ifndef RULES_TABLE_HPP
#define RULES_TABLE_HPP

#include <boost/preprocessor/repetition.hpp> 
#include <boost/preprocessor/iteration/local.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/facilities/identity.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/facilities/empty.hpp>

#include <map>
#include<sstream>

#ifndef MAX_RULES_TABLE_PARAMS
#define MAX_RULES_TABLE_PARAMS 10 
#endif

enum RulesTableReturn
{
	RULES_TABLE_NOT_FOUND = -1,
	RULES_TABLE_FOUND = 0,
	RULES_TABLE_FOUND_DEFAULT = 1
};
enum RulesTableStatus
{
	RULES_TABLE_STATUS_NO_DEFAULT, 
	RULES_TABLE_STATUS_WITH_DEFAULT
};

template<typename Value>
class RulesTable0
{
public:
	RulesTable0(){}
	RulesTable0(const Value value):_value(value){}
	RulesTable0(const RulesTable0<Value>& table):_value(table.getRule()){}
	RulesTable0<Value> operator = (const RulesTable0<Value>& table){_value = table.getRule();return *this;}
	RulesTable0<Value> operator = (const Value& value){_value = value;return *this;}
	operator Value() {return _value;}

	void setRule(const Value& value)
	{
		_value = value;
	}
	
	int getRule(Value& value)
	{
		value = _value;
		return RULES_TABLE_FOUND;
	}

	Value getRule() const 
	{
		return _value;
	}

	static void toDefaultString(std::ostringstream& os, int nTab, Value* value)
	{
		os << "=>\t";
		if (NULL != value)
		{
			os << *value ;
		}
		os << std::endl;
	}
	void toString(std::ostringstream& os, int nTab, Value* value)
	{
		os << "=>\t" << _value << std::endl;
	}

	template <typename StreamType>
	friend StreamType & operator << (StreamType& os, RulesTable0<Value>& rulesTable)
	{
		os << rulesTable._value; 
		return os;
	}

private:
	Value _value;
};

//terible thing begins

//set default value, n*n complex, split a 'n' here :-)
#define DECL_SET_DEFAULT_VALUE(z, m, unused) \
	void setDefaultRule(BOOST_PP_ENUM_BINARY_PARAMS(m, const Key, &key) BOOST_PP_COMMA_IF(m) const Value& value)\
	{\
		BOOST_PP_IF(m, \
		BOOST_PP_IDENTITY(this->_rules[key0].setDefaultRule(BOOST_PP_ENUM_SHIFTED_PARAMS(m, key) BOOST_PP_COMMA_IF(BOOST_PP_DEC(m)) value);), \
		BOOST_PP_IDENTITY(_value = value;_status = RULES_TABLE_STATUS_WITH_DEFAULT;))()\
	}\



#define DECL_RULES_TABLE(z, n, unused) \
template <BOOST_PP_ENUM_PARAMS(n, typename Key), typename Value> \
class BOOST_PP_CAT(RulesTable, n) \
{ \
public: \
typedef BOOST_PP_CAT(RulesTable, BOOST_PP_DEC(n))<BOOST_PP_ENUM_SHIFTED_PARAMS(n, Key) BOOST_PP_COMMA_IF(BOOST_PP_DEC(n)) Value >  SUB_MAP_TYPE;  \
\
	BOOST_PP_CAT(RulesTable, n)() \
		: _status(RULES_TABLE_STATUS_NO_DEFAULT) \
	{}\
    \
    void clear() \
    {\
        _status = RULES_TABLE_STATUS_NO_DEFAULT;\
        _rules.clear();\
    }\
\
	template <typename StreamType> \
	friend StreamType & operator << (StreamType& os, BOOST_PP_CAT(RulesTable, n)<BOOST_PP_ENUM_PARAMS(n, Key), Value>& rulesTable)\
	{\
		std::ostringstream ostr;\
		rulesTable.toString(ostr, 0, NULL);\
		os << ostr.str();\
		return  os;\
	}\
	void toString(std::ostringstream& os, int nTab, Value* value)\
	{\
		if (NULL != value) \
		{\
			toDefaultString(os, nTab, value);\
			return;\
		}\
		int first = 1;\
		typename std::map<Key0, SUB_MAP_TYPE>::iterator it = _rules.begin();\
		typename std::map<Key0, SUB_MAP_TYPE>::iterator end = _rules.end();\
		for(;it != end; it++) \
		{\
			if (!first)\
			{\
				for(int i = 0; i < nTab; i++){os << "[-]\t"; }\
			}\
			os << "[" << it->first <<"]\t";\
			it->second.toString(os, nTab + 1, value);\
			first = 0;\
		}\
		if (RULES_TABLE_STATUS_WITH_DEFAULT == _status)\
		{\
			if (!first)\
			{\
				for(int i = 0; i < nTab; i++){os << "[-]\t"; }\
			}\
			toDefaultString(os, nTab, &_value);\
		}\
	}\
	static void toDefaultString(std::ostringstream& os, int nTab, Value* value)\
	{\
		os << "[N/A]\t";\
		SUB_MAP_TYPE::toDefaultString(os, nTab + 1, value);\
	}\
\
	void setRule(BOOST_PP_ENUM_BINARY_PARAMS(n, const Key, &key), const Value& value)\
	{\
		_rules[key0].setRule(BOOST_PP_ENUM_SHIFTED_PARAMS(n, key) BOOST_PP_COMMA_IF(BOOST_PP_DEC(n)) value); \
	}\
\
BOOST_PP_REPEAT(n,DECL_SET_DEFAULT_VALUE, ~)\
\
	int getRule(BOOST_PP_ENUM_BINARY_PARAMS(n, const Key, &key), Value& value)\
	{\
		int ret = RULES_TABLE_NOT_FOUND; \
		typename std::map<Key0, SUB_MAP_TYPE>::iterator lb = _rules.find(key0); \
		if (lb != _rules.end())\
		{\
			ret = lb->second.getRule(BOOST_PP_ENUM_SHIFTED_PARAMS(n, key) BOOST_PP_COMMA_IF(BOOST_PP_DEC(n)) value); \
		}\
		else \
		{\
			ret = RULES_TABLE_NOT_FOUND; \
		}\
		if (RULES_TABLE_NOT_FOUND == ret && RULES_TABLE_STATUS_WITH_DEFAULT  == _status)\
		{\
			value = _value;\
			ret = RULES_TABLE_FOUND_DEFAULT; \
		}\
		return ret;\
	}\
	Value getRule(BOOST_PP_ENUM_BINARY_PARAMS(n, const Key, &key)) \
	{\
		Value value;\
		getRule(BOOST_PP_ENUM_PARAMS(n, key), value);\
		return value;\
	}\
\
	SUB_MAP_TYPE& operator[] (const Key0& key0) {return _rules[key0];} \
private:\
	std::map<Key0, SUB_MAP_TYPE> _rules; \
	Value _value; \
	int   _status; \
}; \

BOOST_PP_REPEAT_FROM_TO(1, MAX_RULES_TABLE_PARAMS, DECL_RULES_TABLE, ~)
//#define BOOST_PP_LOCAL_MACRO (n) DECL_RULES_TABLE(~, n, ~)
//#define BOOST_PP_LOCAL_LIMITS (1, MAX_RULES_TABLE_PARAMS)
//#include BOOST_PP_LOCAL_ITERATE()

#endif
