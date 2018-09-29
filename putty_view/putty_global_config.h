#ifndef PUTTY_GLOBAL_CONFIG_H
#define PUTTY_GLOBAL_CONFIG_H


#include "base/memory/singleton.h"
#include "RulesTable.hpp"

#include "putty.h"

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

	void initShortcutRules()
	{
		shortcutsRules_.clear();
		shortcutsRules_.setDefaultRule(NULL);
		char keyBuf[128] = { 0 };
		for (int i = 0; i < all_mainwin_shortcut_key_count; i++)
		{
			const char* shortcutName = all_mainwin_shortcut_key_str[i];
			snprintf(keyBuf, sizeof(keyBuf)-1, "%s%s", shortcutName, "Enable");
			int enabled = load_global_isetting(keyBuf, 1);
			if (enabled == 0){ continue; }

			snprintf(keyBuf, sizeof(keyBuf)-1, "%s%s", shortcutName, "Type");
			int keyType = load_global_isetting(keyBuf, get_default_shortcut_keytype(shortcutName));
			if (((1 << keyType) & MASK_NO_FN) == 0)//fn
			{
				shortcutsRules_.setDefaultRule(keyType, shortcutName);
				continue;
			}

			if (!strcmp(shortcutName, SHORTCUT_KEY_SELECT_TAB)){
				for (int c = '0'; c <= '9'; c++){
					shortcutsRules_.setRule(keyType, c, SHORTCUT_KEY_SELECT_TAB);
				}
				continue;
			}

			snprintf(keyBuf, sizeof(keyBuf)-1, "%s%s", shortcutName, "Key");
			int keyVal = load_global_isetting(keyBuf, get_default_shortcut_keyval(shortcutName));
			if (keyVal <= 0){ continue; }
			shortcutsRules_.setRule(keyType, keyVal, shortcutName);
		}
	}

	const char* getShortcutRules(int keyType, int keyVal)
	{
		return shortcutsRules_.getRule(keyType, keyVal);
	}

private:
	bool isShotcutKeyEnabled_;
	RulesTable2<int, int, const char*> shortcutsRules_;
	
	friend struct DefaultSingletonTraits<PuttyGlobalConfig>;
	DISALLOW_COPY_AND_ASSIGN(PuttyGlobalConfig);
};

#endif /* PUTTY_GLOBAL_CONFIG_H */
