/*
 * uxstore.c: Unix-specific implementation of the interface defined
 * in storage.h.
 */
#ifdef WINNT
typedef long int off_t;
#else
#include <pwd.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "putty.h"
#include "storage.h"
#include "tree234.h"

#include <sstream>
using namespace std;

#include <map>
#include <string>
extern std::map<std::string, int> DEFAULT_INT_VALUE;
extern std::map<std::string, std::string> DEFAULT_STR_VALUE;

#ifdef PATH_MAX
#define FNLEN PATH_MAX
#else
#define FNLEN 1024 /* XXX */
#endif

enum {
    INDEX_DIR, INDEX_HOSTKEYS, INDEX_HOSTKEYS_TMP, INDEX_RANDSEED,
    INDEX_SESSIONDIR, INDEX_SESSION,
};

static const char *const puttystr = PUTTY_REG_POS "\\Sessions";

static const char hex_str[17] = "0123456789ABCDEF";

struct WriteHandler
{
	void *parentConfM;
	void* implStoreHandleM;
};

void *TmplStore::open_settings_w(const char *sessionname, char **errmsg)
{
	void * implStoreHandle = implStorageM->open_settings_w(sessionname, errmsg);
	if (implStoreHandle == NULL){ return NULL; }

	WriteHandler* handler = new WriteHandler;
	handler->implStoreHandleM = implStoreHandle;
	if (strcmp(sessionname, DEFAULT_SESSION_NAME) == 0)
	{
		handler->parentConfM = NULL;
		return handler;
	}
	
	char* ch = NULL;
	char loading_session[4096] = { 0 };
	assert(strlen(sessionname) < sizeof(loading_session));
	strcpy(loading_session, sessionname);
	if (strlen(loading_session) > 0){ loading_session[strlen(loading_session) - 1] = '\0'; }
	if ((ch = strrchr(loading_session, '#')) != NULL)
	{
		*ch = '\0';
		handler->parentConfM = open_settings_r(loading_session);
		return handler;
	}
	else
	{
		handler->parentConfM = open_settings_r(DEFAULT_SESSION_NAME);
		return handler;
	}
	return handler;
}

void TmplStore::write_setting_s(void *handle, const char *key, const char *value)
{
	if (handle == NULL){ return; }
	WriteHandler* hd = (WriteHandler*)handle;
	HKEY hkey = (HKEY)hd->implStoreHandleM;
	tree234 *tree = (tree234 *)hd->parentConfM;
	if (tree == NULL)
	{
		if (is_default_value(key, value)){ return; }
	}
	else
	{
		struct skeyval tmp, *kv;
		tmp.key = key;

		if ((kv = (struct skeyval*)find234(tree, &tmp, NULL)) != NULL) {
			if (strcmp(kv->value, value) == 0) { return; }
		}
		if (is_default_value(key, value)){ return; }
	}
	RegSetValueEx(hkey, key, 0, REG_SZ, (BYTE*)value, 1 + strlen(value));
}

void TmplStore::write_setting_i(void *handle, const char *key, int value)
{
	if (handle == NULL){ return; }
	WriteHandler* hd = (WriteHandler*)handle;
	HKEY hkey = (HKEY)hd->implStoreHandleM;
	tree234 *tree = (tree234 *)hd->parentConfM;
	if (tree == NULL)
	{
		if (is_default_value(key, value)){ return; }
	}
	else
	{
		struct skeyval tmp, *kv;
		tmp.key = key;

		if ((kv = (struct skeyval*)find234(tree, &tmp, NULL)) != NULL) {
			int val = atoi(kv->value);
			if (val == value) { return; }
		}
		if (is_default_value(key, value)){ return; }
	}

	RegSetValueEx((HKEY)handle, key, 0, REG_DWORD, (CONST BYTE *) &value, sizeof(value));
}

void TmplStore::close_settings_w(void *handle)
{
	if (handle == NULL){ return; }
	WriteHandler* hd = (WriteHandler*)handle;

	if (hd->implStoreHandleM)
	{
		implStorageM->close_settings_w(hd->implStoreHandleM);
	}
	if (hd->parentConfM)
	{
		close_settings_r(hd->parentConfM);
	}
	delete handle;
}


void *TmplStore::open_settings_r(const char *sessionname)
{
	char loading_session[4096] = { 0 };
    tree234 *ret = newtree234(keycmp);

	if (sessionname == NULL)
	{
		strcpy(loading_session, DEFAULT_SESSION_NAME);
	}
	else
	{
		assert(strlen(sessionname) < sizeof(loading_session));
		strcpy(loading_session, sessionname);
	}
	implStorageM->load_settings_to_tree234(loading_session, ret);
	if (strlen(loading_session) > 0){ loading_session[strlen(loading_session) - 1] = '\0'; }

	char* ch;
	while ((ch = strrchr(loading_session, '#')) != NULL)
	{
		*(ch + 1) = '\0';
		implStorageM->load_settings_to_tree234(loading_session, ret);
		*ch = '\0';
	}

	if (sessionname != NULL && strcmp(sessionname, DEFAULT_SESSION_NAME) != 0)
	{
		implStorageM->load_settings_to_tree234(DEFAULT_SESSION_NAME, ret);
	}

    return ret;
}

char *TmplStore::read_setting_s(void *handle, const char *key, char *buffer, int buflen)
{
    tree234 *tree = (tree234 *)handle;
    const char *val = NULL;
    struct skeyval tmp, *kv;

    tmp.key = key;
    if (tree != NULL &&
        (kv = (struct skeyval*)find234(tree, &tmp, NULL)) != NULL) {
        val = kv->value;
        assert(val != NULL);
    } 

    if (!val)
	return NULL;
    else {
	strncpy(buffer, val, buflen);
	buffer[buflen-1] = '\0';
	return buffer;
    }
}

int TmplStore::read_setting_i(void *handle, const char *key, int defvalue)
{
    tree234 *tree = (tree234 *)handle;
    const char *val = NULL;
    struct skeyval tmp, *kv;

    tmp.key = key;
    if (tree != NULL &&
        (kv = (struct skeyval*)find234(tree, &tmp, NULL)) != NULL) {
        val = kv->value;
        assert(val != NULL);
    }

    if (!val)
	return defvalue;
    else
	return atoi(val);
}

FontSpec * TmplStore::read_setting_fontspec(void *handle, const char *name)
{
    /*
     * In GTK1-only PuTTY, we used to store font names simply as a
     * valid X font description string (logical or alias), under a
     * bare key such as "Font".
     * 
     * In GTK2 PuTTY, we have a prefix system where "client:"
     * indicates a Pango font and "server:" an X one; existing
     * configuration needs to be reinterpreted as having the
     * "server:" prefix, so we change the storage key from the
     * provided name string (e.g. "Font") to a suffixed one
     * ("FontName").
     */
    char *settingname;
	char* font_name = NULL;
	int isbold = 0, charset = 0, height = 0;

    if (!(font_name = IStore::read_setting_s(handle, name))){
		char *suffname = dupcat(name, "Name", NULL);
	    if (font_name = IStore::read_setting_s(handle, suffname)) {
			/* got new-style name */
			sfree(suffname);
	    }else{
	    	sfree(suffname);
		    return NULL;
	    }
    }
    settingname = dupcat(name, "IsBold", NULL);
    isbold = read_setting_i(handle, settingname, -1);
    sfree(settingname);
    if (isbold == -1) return 0;

    settingname = dupcat(name, "CharSet", NULL);
    charset = read_setting_i(handle, settingname, -1);
    sfree(settingname);
    if (charset == -1) return 0;

    settingname = dupcat(name, "Height", NULL);
    height = read_setting_i(handle, settingname, INT_MIN);
    sfree(settingname);
    if (height == INT_MIN) return 0;

	return fontspec_new(font_name, isbold, height, charset);
}

Filename *TmplStore::read_setting_filename(void *handle, const char *name)
{
	char* path = IStore::read_setting_s(handle, name);
	if (path)
	{
		Filename* ret = new Filename();
		ret->path = path;
		return ret;
	}
    return NULL;
}

void TmplStore::write_setting_fontspec(void *handle, const char *name, FontSpec* result)
{
	if (result == NULL)return;
    /*
     * read_setting_fontspec had to handle two cases, but when
     * writing our settings back out we simply always generate the
     * new-style name.
     */
    char *suffname = dupcat(name, "Name", NULL);
    write_setting_s(handle, suffname, result->name);
    sfree(suffname);
	
	char *settingname;
    write_setting_s(handle, name, result->name);
    settingname = dupcat(name, "IsBold", NULL);
    write_setting_i(handle, settingname, result->isbold);
    sfree(settingname);
    settingname = dupcat(name, "CharSet", NULL);
    write_setting_i(handle, settingname, result->charset);
    sfree(settingname);
    settingname = dupcat(name, "Height", NULL);
    write_setting_i(handle, settingname, result->height);
    sfree(settingname);
}

void TmplStore::write_setting_filename(void *handle, const char *name, Filename* result)
{
	if (result)
	{
		write_setting_s(handle, name, result->path);
	}
}

void TmplStore::close_settings_r(void *handle)
{
    tree234 *tree = (tree234 *)handle;
    struct skeyval *kv;

    if (!tree)
        return;

    while ( (kv = (struct skeyval*)index234(tree, 0)) != NULL) {
        del234(tree, kv);
        sfree((char *)kv->key);
        sfree((char *)kv->value);
        sfree(kv);
    }

    freetree234(tree);
}

void TmplStore::del_settings(const char *sessionname)
{
	implStorageM->del_settings(sessionname);
}

void *TmplStore::enum_settings_start(void)
{
	return implStorageM->enum_settings_start();
}

char *TmplStore::enum_settings_next(void *handle, char *buffer, int buflen)
{
	return implStorageM->enum_settings_next(handle, buffer, buflen);
}

void TmplStore::enum_settings_finish(void *handle)
{
	implStorageM->enum_settings_finish(handle);
}

int TmplStore::verify_host_key(const char *hostname, int port,
		    const char *keytype, const char *key)
{ 
	return implStorageM->verify_host_key(hostname, port, keytype, key);
}

void TmplStore::store_host_key(const char *hostname, int port,
		    const char *keytype, const char *key)
{   
	implStorageM->verify_host_key(hostname, port, keytype, key);
}

void TmplStore::read_random_seed(noise_consumer_t consumer)
{
	implStorageM->read_random_seed(consumer);
}

void TmplStore::write_random_seed(void *data, int len)
{
	implStorageM->write_random_seed(data, len);
}

void TmplStore::cleanup_all(void)
{
	implStorageM->cleanup_all();
}
