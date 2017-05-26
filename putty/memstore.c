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

MemStore gMemStore;

enum {
    INDEX_DIR, INDEX_HOSTKEYS, INDEX_HOSTKEYS_TMP, INDEX_RANDSEED,
    INDEX_SESSIONDIR, INDEX_SESSION,
};
extern Conf* DEFAULT_CFG;

void MemStore::input(const char* input)
{
	inputM = input;
}

static const char hex_str[17] = "0123456789ABCDEF";

char *MemStore::mungestr(const char *in)
{
    char *out, *ret;

    if (!in || !*in)
        in = DEFAULT_SESSION_NAME;

    ret = out = snewn(3*strlen(in)+1, char);

    while (*in) {
        /*
         * There are remarkably few punctuation characters that
         * aren't shell-special in some way or likely to be used as
         * separators in some file format or another! Hence we use
         * opt-in for safe characters rather than opt-out for
         * specific unsafe ones...
         */
	if (*in!='+' && *in!='-' && *in!='.' && *in!='@' && *in!='_' &&
            !(*in >= '0' && *in <= '9') &&
            !(*in >= 'A' && *in <= 'Z') &&
            !(*in >= 'a' && *in <= 'z')) {
	    *out++ = '%';
		*out++ = hex_str[((unsigned char)*in) >> 4];
		*out++ = hex_str[((unsigned char)*in) & 15];
	} else
	    *out++ = *in;
	in++;
    }
    *out = '\0';
    return ret;
}

char *MemStore::unmungestr(const char *in)
{
    char *out, *ret;
    out = ret = snewn(strlen(in)+1, char);
    while (*in) {
	if (*in == '%' && in[1] && in[2]) {
	    int i, j;

	    i = in[1] - '0';
	    i -= (i > 9 ? 7 : 0);
	    j = in[2] - '0';
	    j -= (j > 9 ? 7 : 0);

	    *out++ = (i << 4) + j;
	    in += 3;
	} else {
	    *out++ = *in++;
	}
    }
    *out = '\0';
    return ret;
}

void *MemStore::open_settings_w(const char *sessionname, char **errmsg)
{
	stringstream* memio = new stringstream();
	return memio;
}

void MemStore::write_setting_s(void *handle, const char *key, const char *value)
{
	if (DEFAULT_STR_VALUE[key] == value){ return; }
    stringstream *fp = (stringstream *)handle;
	(*fp) << key << "=" << value << "\n";
}

void MemStore::write_setting_i(void *handle, const char *key, int value)
{
	if (DEFAULT_INT_VALUE[key] == value){ return; }
	stringstream *fp = (stringstream *)handle;
	(*fp) << key << "=" << value << "\n";
}

void MemStore::close_settings_w(void *handle)
{
    stringstream *fp = (stringstream *)handle;
    delete (fp);
}

/*
 * Reading settings, for the moment, is done by retrieving X
 * resources from the X display. When we introduce disk files, I
 * think what will happen is that the X resources will override
 * PuTTY's inbuilt defaults, but that the disk files will then
 * override those. This isn't optimal, but it's the best I can
 * immediately work out.
 * FIXME: the above comment is a bit out of date. Did it happen?
 */

struct skeyval {
    const char *key;
    const char *value;
};

static tree234 *xrmtree = NULL;

static int keycmp(void *av, void *bv)
{
    struct skeyval *a = (struct skeyval *)av;
    struct skeyval *b = (struct skeyval *)bv;
    return strcmp(a->key, b->key);
}

static void provide_xrm_string(char *string)
{
    char *p, *q, *key;
    struct skeyval *xrms, *ret;

    p = q = strchr(string, ':');
    if (!q) {
	fprintf(stderr, "pterm: expected a colon in resource string"
		" \"%s\"\n", string);
	return;
    }
    q++;
    while (p > string && p[-1] != '.' && p[-1] != '*')
	p--;
    xrms = snew(struct skeyval);
    key = snewn(q-p, char);
    memcpy(key, p, q-p);
    key[q-p-1] = '\0';
    xrms->key = key;
    while (*q && isspace((unsigned char)*q))
	q++;
    xrms->value = dupstr(q);

    if (!xrmtree)
	xrmtree = newtree234(keycmp);

    ret = (struct skeyval*)add234(xrmtree, xrms);
    if (ret) {
	/* Override an existing string. */
	del234(xrmtree, ret);
	add234(xrmtree, xrms);
    }
}

static const char *get_setting(const char *key)
{
    struct skeyval tmp, *ret;
    tmp.key = key;
    if (xrmtree) {
	ret = (skeyval*)find234(xrmtree, &tmp, NULL);
	if (ret)
	    return ret->value;
    }
#ifdef WINNT
	return NULL;
#else
    return x_get_default(key);
#endif
}

void *MemStore::open_settings_r(const char *sessionname)
{
	char line[4096] = { 0 };
    tree234 *ret;

	if (inputM == NULL){ return NULL; }
    stringstream fp(inputM);
	
    ret = newtree234(keycmp);

	while ((fp.getline(line, sizeof(line)))) {
        char *value = strchr(line, '=');
        struct skeyval *kv;

        if (!value)
            continue;
        *value++ = '\0';
        value[strcspn(value, "\r\n")] = '\0';   /* trim trailing NL */

        kv = snew(struct skeyval);
        kv->key = dupstr(line);
        kv->value = dupstr(value);
        add234(ret, kv);
    }
    return ret;
}

char *MemStore::read_setting_s(void *handle, const char *key, char *buffer, int buflen)
{
    tree234 *tree = (tree234 *)handle;
    const char *val;
    struct skeyval tmp, *kv;

    tmp.key = key;
    if (tree != NULL &&
        (kv = (struct skeyval*)find234(tree, &tmp, NULL)) != NULL) {
        val = kv->value;
        assert(val != NULL);
    } else
        val = get_setting(key);

    if (!val)
	return NULL;
    else {
	strncpy(buffer, val, buflen);
	buffer[buflen-1] = '\0';
	return buffer;
    }
}

int MemStore::read_setting_i(void *handle, const char *key, int defvalue)
{
    tree234 *tree = (tree234 *)handle;
    const char *val;
    struct skeyval tmp, *kv;

    tmp.key = key;
    if (tree != NULL &&
        (kv = (struct skeyval*)find234(tree, &tmp, NULL)) != NULL) {
        val = kv->value;
        assert(val != NULL);
    } else
        val = get_setting(key);

    if (!val)
	return defvalue;
    else
	return atoi(val);
}

FontSpec * MemStore::read_setting_fontspec(void *handle, const char *name)
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

Filename *MemStore::read_setting_filename(void *handle, const char *name)
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

void MemStore::write_setting_fontspec(void *handle, const char *name, FontSpec* result)
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

void MemStore::write_setting_filename(void *handle, const char *name, Filename* result)
{
	if (result)
	{
		write_setting_s(handle, name, result->path);
	}
}

void MemStore::close_settings_r(void *handle)
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

void MemStore::del_settings(const char *sessionname)
{
}

void *MemStore::enum_settings_start(void)
{
	return NULL;
}

char *MemStore::enum_settings_next(void *handle, char *buffer, int buflen)
{
    return NULL;
}

void MemStore::enum_settings_finish(void *handle)
{
}

int MemStore::verify_host_key(const char *hostname, int port,
		    const char *keytype, const char *key)
{ 
    return 2;
}

void MemStore::store_host_key(const char *hostname, int port,
		    const char *keytype, const char *key)
{   
}

void MemStore::read_random_seed(noise_consumer_t consumer)
{
}

void MemStore::write_random_seed(void *data, int len)
{
}

void MemStore::cleanup_all(void)
{
}
