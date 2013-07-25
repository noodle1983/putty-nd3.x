#ifndef PUTTY_GLOBAL_CONFIG_H
#define PUTTY_GLOBAL_CONFIG_H


#include "base/memory/singleton.h"

class PuttyGlobalConfig
{
public:
	PuttyGlobalConfig()
		:isShotcutKeyEnabled_(true)
	{}
	~PuttyGlobalConfig(){}
	
	static PuttyGlobalConfig* GetInstance(){
		return Singleton<PuttyGlobalConfig>::get();
	}

	bool isShotcutKeyEnabled(){
		return isShotcutKeyEnabled_;
	}
	void setShotcutKeyEnabled(bool enabled){
		isShotcutKeyEnabled_ = enabled;
	}

private:
	bool isShotcutKeyEnabled_;
	
	friend struct DefaultSingletonTraits<PuttyGlobalConfig>;
	DISALLOW_COPY_AND_ASSIGN(PuttyGlobalConfig);
};

#endif /* PUTTY_GLOBAL_CONFIG_H */
