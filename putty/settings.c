/*
 * settings.c: read and write saved sessions. (platform-independent)
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "putty.h"
#include "storage.h"
#include "des.h"

#include <map>
#include <string>
std::map<std::string, int> DEFAULT_INT_VALUE;
std::map<std::string, std::string> DEFAULT_STR_VALUE;
bool isInited = false;

/* The cipher order given here is the default order. */
static const struct keyvalwhere ciphernames[] = {
    { "chacha20",   CIPHER_CHACHA20,        -1, -1 },
    { "aes",        CIPHER_AES,             -1, -1 },
    { "blowfish",   CIPHER_BLOWFISH,        -1, -1 },
    { "3des",       CIPHER_3DES,            -1, -1 },
    { "WARN",       CIPHER_WARN,            -1, -1 },
    { "arcfour",    CIPHER_ARCFOUR,         -1, -1 },
    { "des",        CIPHER_DES,             -1, -1 }
};

static const struct keyvalwhere kexnames[] = {
    { "ecdh",               KEX_ECDH,       -1, +1 },
    { "dh-gex-sha1",        KEX_DHGEX,      -1, -1 },
    { "dh-group14-sha1",    KEX_DHGROUP14,  -1, -1 },
    { "dh-group1-sha1",     KEX_DHGROUP1,   -1, -1 },
    { "rsa",                KEX_RSA,        KEX_WARN, -1 },
    { "WARN",               KEX_WARN,       -1, -1 }
};

/*
 * All the terminal modes that we know about for the "TerminalModes"
 * setting. (Also used by config.c for the drop-down list.)
 * This is currently precisely the same as the set in ssh.c, but could
 * in principle differ if other backends started to support tty modes
 * (e.g., the pty backend).
 */
const char *const ttymodes[] = {
    "INTR",	"QUIT",     "ERASE",	"KILL",     "EOF",
    "EOL",	"EOL2",     "START",	"STOP",     "SUSP",
    "DSUSP",	"REPRINT",  "WERASE",	"LNEXT",    "FLUSH",
    "SWTCH",	"STATUS",   "DISCARD",	"IGNPAR",   "PARMRK",
    "INPCK",	"ISTRIP",   "INLCR",	"IGNCR",    "ICRNL",
    "IUCLC",	"IXON",     "IXANY",	"IXOFF",    "IMAXBEL",
    "ISIG",	"ICANON",   "XCASE",	"ECHO",     "ECHOE",
    "ECHOK",	"ECHONL",   "NOFLSH",	"TOSTOP",   "IEXTEN",
    "ECHOCTL",	"ECHOKE",   "PENDIN",	"OPOST",    "OLCUC",
    "ONLCR",	"OCRNL",    "ONOCR",	"ONLRET",   "CS7",
    "CS8",	"PARENB",   "PARODD",	NULL
};

/*
 * Convenience functions to access the backends[] array
 * (which is only present in tools that manage settings).
 */

Backend *backend_from_name(const char *name)
{
    Backend **p;
    for (p = backends; *p != NULL; p++)
	if (!strcmp((*p)->name, name))
	    return *p;
    return NULL;
}

Backend *backend_from_proto(int proto)
{
    Backend **p;
    for (p = backends; *p != NULL; p++)
	if ((*p)->protocol == proto)
	    return *p;
    return NULL;
}

char *get_remote_username(Conf *conf)
{
    char *username = conf_get_str(conf, CONF_username);
    if (*username) {
	return dupstr(username);
    } else if (conf_get_int(conf, CONF_username_from_env)) {
	/* Use local username. */
	return get_username();     /* might still be NULL */
    } else {
	return NULL;
    }
}

static char *gpps_raw(IStore* iStorage, void *handle, const char *name, const char *def)
{
	if (!isInited && def != NULL){ DEFAULT_STR_VALUE[name] = def; }
    char *ret = iStorage->read_setting_s(handle, name);
    if (!ret)
	ret = platform_default_s(name);
    if (!ret)
	ret = def ? dupstr(def) : NULL;   /* permit NULL as final fallback */
    return ret;
}

static void gpps(IStore* iStorage, void *handle, const char *name, const char *def,
		 Conf *conf, int primary)
{
    char *val = gpps_raw(iStorage, handle, name, def);
    conf_set_str(conf, primary, val);
    sfree(val);
}

static void gpps_s(IStore* iStorage, void *handle, const char *name, const char *def,
		 Conf *conf, int primary, int subkey)
{
    char *val = gpps_raw(iStorage, handle, name, def);
	conf_set_int_str(conf, primary, subkey, val);
    sfree(val);
}
/*
 * gppfont and gppfile cannot have local defaults, since the very
 * format of a Filename or FontSpec is platform-dependent. So the
 * platform-dependent functions MUST return some sort of value.
 */
static void gppfont(IStore* iStorage, void *handle, const char *name, Conf *conf, int primary)
{
    FontSpec *result = iStorage->read_setting_fontspec(handle, name);
    if (!result)
        result = platform_default_fontspec(name);
    conf_set_fontspec(conf, primary, result);
    fontspec_free(result);
}
static void gppfile(IStore* iStorage, void *handle, const char *name, Conf *conf, int primary)
{
    Filename *result = iStorage->read_setting_filename(handle, name);
	if (!result)
	{
		result = platform_default_filename(name);
	}
    conf_set_filename(conf, primary, result);
    filename_free(result);
}

static int gppi_raw(IStore* iStorage, void *handle, const char *name, int def)
{
	if (!isInited){ DEFAULT_INT_VALUE[name] = def; }
    def = platform_default_i(name, def);
    return iStorage->read_setting_i(handle, name, def);
}

static void gppi(IStore* iStorage, void *handle, const char *name, int def,
                 Conf *conf, int primary)
{
    conf_set_int(conf, primary, gppi_raw(iStorage, handle, name, def));
}

static void gppi_i(IStore* iStorage, void *handle, const char *name, int def,
                 Conf *conf, int primary, int subkey)
{
    conf_set_int_int(conf, primary, subkey, gppi_raw(iStorage, handle, name, def));
}
/*
 * Read a set of name-value pairs in the format we occasionally use:
 *   NAME\tVALUE\0NAME\tVALUE\0\0 in memory
 *   NAME=VALUE,NAME=VALUE, in storage
 * If there's no "=VALUE" (e.g. just NAME,NAME,NAME) then those keys
 * are mapped to the empty string.
 */
static int gppmap(IStore* iStorage, void *handle, const char *name, Conf *conf, int primary)
{
    char *buf, *p, *q, *key, *val;

    /*
     * Start by clearing any existing subkeys of this key from conf.
     */
    while ((key = conf_get_str_nthstrkey(conf, primary, 0)) != NULL)
        conf_del_str_str(conf, primary, key);

    /*
     * Now read a serialised list from the settings and unmarshal it
     * into its components.
     */
    buf = gpps_raw(iStorage, handle, name, NULL);
    if (!buf)
	return FALSE;

    p = buf;
    while (*p) {
	q = buf;
	val = NULL;
	while (*p && *p != ',') {
	    int c = *p++;
	    if (c == '=')
		c = '\0';
	    if (c == '\\')
		c = *p++;
	    *q++ = c;
	    if (!c)
		val = q;
	}
	if (*p == ',')
	    p++;
	if (!val)
	    val = q;
	*q = '\0';

        if (primary == CONF_portfwd && strchr(buf, 'D') != NULL) {
            /*
             * Backwards-compatibility hack: dynamic forwardings are
             * indexed in the data store as a third type letter in the
             * key, 'D' alongside 'L' and 'R' - but really, they
             * should be filed under 'L' with a special _value_,
             * because local and dynamic forwardings both involve
             * _listening_ on a local port, and are hence mutually
             * exclusive on the same port number. So here we translate
             * the legacy storage format into the sensible internal
             * form, by finding the D and turning it into a L.
             */
            char *newkey = dupstr(buf);
            *strchr(newkey, 'D') = 'L';
            conf_set_str_str(conf, primary, newkey, "D");
            sfree(newkey);
        } else {
            conf_set_str_str(conf, primary, buf, val);
        }
    }
    sfree(buf);

    return TRUE;
}

/*
 * Write a set of name/value pairs in the above format, or just the
 * names if include_values is FALSE.
 */
static void wmap(IStore* iStorage, void *handle, char const *outkey, Conf *conf, int primary,
                 int include_values)
{
    char *buf, *p, *key, *realkey;
    const char *val, *q;
    int len;

    len = 1;			       /* allow for NUL */

    for (val = conf_get_str_strs(conf, primary, NULL, &key);
	 val != NULL;
	 val = conf_get_str_strs(conf, primary, key, &key))
	len += 2 + 2 * (strlen(key) + strlen(val));   /* allow for escaping */

    buf = snewn(len, char);
    p = buf;

    for (val = conf_get_str_strs(conf, primary, NULL, &key);
	 val != NULL;
	 val = conf_get_str_strs(conf, primary, key, &key)) {

        if (primary == CONF_portfwd && !strcmp(val, "D")) {
            /*
             * Backwards-compatibility hack, as above: translate from
             * the sensible internal representation of dynamic
             * forwardings (key "L<port>", value "D") to the
             * conceptually incoherent legacy storage format (key
             * "D<port>", value empty).
             */
            char *L;

            realkey = key;             /* restore it at end of loop */
            val = "";
            key = dupstr(key);
            L = strchr(key, 'L');
            if (L) *L = 'D';
        } else {
            realkey = NULL;
        }

	if (p != buf)
	    *p++ = ',';
	for (q = key; *q; q++) {
	    if (*q == '=' || *q == ',' || *q == '\\')
		*p++ = '\\';
	    *p++ = *q;
	}
        if (include_values) {
            *p++ = '=';
            for (q = val; *q; q++) {
                if (*q == '=' || *q == ',' || *q == '\\')
                    *p++ = '\\';
                *p++ = *q;
            }
        }

        if (realkey) {
            free(key);
            key = realkey;
        }
    }
    *p = '\0';
    iStorage->write_setting_s(handle, outkey, buf);
    sfree(buf);
}

static int key2val(const struct keyvalwhere *mapping,
                   int nmaps, char *key)
{
    int i;
    for (i = 0; i < nmaps; i++)
	if (!strcmp(mapping[i].s, key)) return mapping[i].v;
    return -1;
}

static const char *val2key(const struct keyvalwhere *mapping,
                           int nmaps, int val)
{
    int i;
    for (i = 0; i < nmaps; i++)
	if (mapping[i].v == val) return mapping[i].s;
    return NULL;
}

/*
 * Helper function to parse a comma-separated list of strings into
 * a preference list array of values. Any missing values are added
 * to the end and duplicates are weeded.
 * XXX: assumes vals in 'mapping' are small +ve integers
 */
static void gprefs(IStore* iStorage, void *sesskey, const char *name, const char *def,
		   const struct keyvalwhere *mapping, int nvals,
		   Conf *conf, int primary)
{
    char *commalist;
    char *p, *q;
    int i, j, n, v, pos;
    unsigned long seen = 0;	       /* bitmap for weeding dups etc */

    /*
     * Fetch the string which we'll parse as a comma-separated list.
     */
    commalist = gpps_raw(iStorage, sesskey, name, def);

    /*
     * Go through that list and convert it into values.
     */
    n = 0;
    p = commalist;
    while (1) {
        while (*p && *p == ',') p++;
        if (!*p)
            break;                     /* no more words */

        q = p;
        while (*p && *p != ',') p++;
        if (*p) *p++ = '\0';

        v = key2val(mapping, nvals, q);
        if (v != -1 && !(seen & (1 << v))) {
	    seen |= (1 << v);
            conf_set_int_int(conf, primary, n, v);
            n++;
	}
    }

    sfree(commalist);

    /*
     * Now go through 'mapping' and add values that weren't mentioned
     * in the list we fetched. We may have to loop over it multiple
     * times so that we add values before other values whose default
     * positions depend on them.
     */
    while (n < nvals) {
        for (i = 0; i < nvals; i++) {
	    assert(mapping[i].v < 32);

	    if (!(seen & (1 << mapping[i].v))) {
                /*
                 * This element needs adding. But can we add it yet?
                 */
                if (mapping[i].vrel != -1 && !(seen & (1 << mapping[i].vrel)))
                    continue;          /* nope */

                /*
                 * OK, we can work out where to add this element, so
                 * do so.
                 */
                if (mapping[i].vrel == -1) {
                    pos = (mapping[i].where < 0 ? n : 0);
                } else {
                    for (j = 0; j < n; j++)
                        if (conf_get_int_int(conf, primary, j) ==
                            mapping[i].vrel)
                            break;
                    assert(j < n);     /* implied by (seen & (1<<vrel)) */
                    pos = (mapping[i].where < 0 ? j : j+1);
                }

                /*
                 * And add it.
                 */
                for (j = n-1; j >= pos; j--)
                    conf_set_int_int(conf, primary, j+1,
                                     conf_get_int_int(conf, primary, j));
                conf_set_int_int(conf, primary, pos, mapping[i].v);
                n++;
            }
        }
    }
}

/* 
 * Write out a preference list.
 */
static void wprefs(IStore* iStorage, void *sesskey, const char *name,
		   const struct keyvalwhere *mapping, int nvals,
		   Conf *conf, int primary)
{
    char *buf, *p;
    int i, maxlen;

    for (maxlen = i = 0; i < nvals; i++) {
	const char *s = val2key(mapping, nvals,
                                conf_get_int_int(conf, primary, i));
	if (s) {
            maxlen += (maxlen > 0 ? 1 : 0) + strlen(s);
        }
    }

    buf = snewn(maxlen + 1, char);
    p = buf;

    for (i = 0; i < nvals; i++) {
	const char *s = val2key(mapping, nvals,
                                conf_get_int_int(conf, primary, i));
	if (s) {
            p += sprintf(p, "%s%s", (p > buf ? "," : ""), s);
	}
    }

    assert(p - buf == maxlen);
    *p = '\0';

    iStorage->write_setting_s(sesskey, name, buf);

    sfree(buf);
}

char *save_settings(const char *section, Conf *conf)
{
    void *sesskey;
    char *errmsg;

	if (section == NULL || section[0] == NULL){ return NULL; }
	int isdef = /*!strcmp(section, DEFAULT_SESSION_NAME)||*/ !strcmp(section, START_LOCAL_SSH_SERVER_NAME) || !strcmp(section, LOCAL_SSH_SESSION_NAME);
	if (isdef){ return NULL; }

	TmplStore tmpl_store(gStorage);
	tmpl_store.del_settings_only(section);
	sesskey = tmpl_store.open_settings_w(section, &errmsg);
    if (!sesskey)
	return errmsg;
	save_open_settings(&tmpl_store, sesskey, conf);
	tmpl_store.close_settings_w(sesskey);
    return NULL;
}

void save_open_settings(IStore* iStorage, void *sesskey, Conf *conf)
{
    int i;
    const char *p;

    iStorage->write_setting_i(sesskey, "Present", 1);
    iStorage->write_setting_s(sesskey, "HostName", conf_get_str(conf, CONF_host));
    iStorage->write_setting_filename(sesskey, "LogFileName", conf_get_filename(conf, CONF_logfilename));
    iStorage->write_setting_i(sesskey, "LogType", conf_get_int(conf, CONF_logtype));
    iStorage->write_setting_i(sesskey, "LogFileClash", conf_get_int(conf, CONF_logxfovr));
    iStorage->write_setting_i(sesskey, "LogFlush", conf_get_int(conf, CONF_logflush));
    iStorage->write_setting_i(sesskey, "SSHLogOmitPasswords", conf_get_int(conf, CONF_logomitpass));
    iStorage->write_setting_i(sesskey, "SSHLogOmitData", conf_get_int(conf, CONF_logomitdata));
    p = "raw";
    {
	const Backend *b = backend_from_proto(conf_get_int(conf, CONF_protocol));
	if (b)
	    p = b->name;
    }
    iStorage->write_setting_s(sesskey, "Protocol", p);
    iStorage->write_setting_i(sesskey, "PortNumber", conf_get_int(conf, CONF_port));
    /* The CloseOnExit numbers are arranged in a different order from
     * the standard FORCE_ON / FORCE_OFF / AUTO. */
    iStorage->write_setting_i(sesskey, "CloseOnExit", (conf_get_int(conf, CONF_close_on_exit)+2)%3);
    iStorage->write_setting_i(sesskey, "WarnOnClose", !!conf_get_int(conf, CONF_warn_on_close));
    iStorage->write_setting_i(sesskey, "PingInterval", conf_get_int(conf, CONF_ping_interval) / 60);	/* minutes */
    iStorage->write_setting_i(sesskey, "PingIntervalSecs", conf_get_int(conf, CONF_ping_interval) % 60);	/* seconds */
    iStorage->write_setting_i(sesskey, "TCPNoDelay", conf_get_int(conf, CONF_tcp_nodelay));
    iStorage->write_setting_i(sesskey, "TCPKeepalives", conf_get_int(conf, CONF_tcp_keepalives));
    iStorage->write_setting_s(sesskey, "TerminalType", conf_get_str(conf, CONF_termtype));
    iStorage->write_setting_s(sesskey, "TerminalSpeed", conf_get_str(conf, CONF_termspeed));
    wmap(iStorage, sesskey, "TerminalModes", conf, CONF_ttymodes, TRUE);

    /* Address family selection */
    iStorage->write_setting_i(sesskey, "AddressFamily", conf_get_int(conf, CONF_addressfamily));

    /* proxy settings */
    iStorage->write_setting_s(sesskey, "ProxyExcludeList", conf_get_str(conf, CONF_proxy_exclude_list));
    iStorage->write_setting_i(sesskey, "ProxyDNS", (conf_get_int(conf, CONF_proxy_dns)+2)%3);
    iStorage->write_setting_i(sesskey, "ProxyLocalhost", conf_get_int(conf, CONF_even_proxy_localhost));
    iStorage->write_setting_i(sesskey, "ProxyMethod", conf_get_int(conf, CONF_proxy_type));
    iStorage->write_setting_s(sesskey, "ProxyHost", conf_get_str(conf, CONF_proxy_host));
    iStorage->write_setting_i(sesskey, "ProxyPort", conf_get_int(conf, CONF_proxy_port));
    iStorage->write_setting_s(sesskey, "ProxyUsername", conf_get_str(conf, CONF_proxy_username));
    iStorage->write_setting_s(sesskey, "ProxyPassword", conf_get_str(conf, CONF_proxy_password));
    iStorage->write_setting_s(sesskey, "ProxyTelnetCommand", conf_get_str(conf, CONF_proxy_telnet_command));
    wmap(iStorage, sesskey, "Environment", conf, CONF_environmt, TRUE);
    iStorage->write_setting_s(sesskey, "UserName", conf_get_str(conf, CONF_username));
    iStorage->write_setting_i(sesskey, "UserNameFromEnvironment", conf_get_int(conf, CONF_username_from_env));
    iStorage->write_setting_s(sesskey, "LocalUserName", conf_get_str(conf, CONF_localusername));
    iStorage->write_setting_i(sesskey, "NoPTY", conf_get_int(conf, CONF_nopty));
    iStorage->write_setting_i(sesskey, "Compression", conf_get_int(conf, CONF_compression));
    iStorage->write_setting_i(sesskey, "TryAgent", conf_get_int(conf, CONF_tryagent));
    iStorage->write_setting_i(sesskey, "AgentFwd", conf_get_int(conf, CONF_agentfwd));
    iStorage->write_setting_i(sesskey, "GssapiFwd", conf_get_int(conf, CONF_gssapifwd));
    iStorage->write_setting_i(sesskey, "ChangeUsername", conf_get_int(conf, CONF_change_username));
    wprefs(iStorage, sesskey, "Cipher", ciphernames, CIPHER_MAX, conf, CONF_ssh_cipherlist);
    wprefs(iStorage, sesskey, "KEX", kexnames, KEX_MAX, conf, CONF_ssh_kexlist);
    iStorage->write_setting_i(sesskey, "RekeyTime", conf_get_int(conf, CONF_ssh_rekey_time));
    iStorage->write_setting_s(sesskey, "RekeyBytes", conf_get_str(conf, CONF_ssh_rekey_data));
    iStorage->write_setting_i(sesskey, "SshNoAuth", conf_get_int(conf, CONF_ssh_no_userauth));
    iStorage->write_setting_i(sesskey, "SshBanner", conf_get_int(conf, CONF_ssh_show_banner));
    iStorage->write_setting_i(sesskey, "AuthTIS", conf_get_int(conf, CONF_try_tis_auth));
    iStorage->write_setting_i(sesskey, "AuthKI", conf_get_int(conf, CONF_try_ki_auth));
    iStorage->write_setting_i(sesskey, "AuthGSSAPI", conf_get_int(conf, CONF_try_gssapi_auth));
#ifndef NO_GSSAPI
    wprefs(iStorage, sesskey, "GSSLibs", gsslibkeywords, ngsslibs, conf, CONF_ssh_gsslist);
    iStorage->write_setting_filename(sesskey, "GSSCustom", conf_get_filename(conf, CONF_ssh_gss_custom));
#endif
    iStorage->write_setting_i(sesskey, "SshNoShell", conf_get_int(conf, CONF_ssh_no_shell));
    iStorage->write_setting_i(sesskey, "SshProt", conf_get_int(conf, CONF_sshprot));
    iStorage->write_setting_s(sesskey, "LogHost", conf_get_str(conf, CONF_loghost));
    iStorage->write_setting_i(sesskey, "SSH2DES", conf_get_int(conf, CONF_ssh2_des_cbc));
    iStorage->write_setting_filename(sesskey, "PublicKeyFile", conf_get_filename(conf, CONF_keyfile));
    iStorage->write_setting_s(sesskey, "RemoteCommand", conf_get_str(conf, CONF_remote_cmd));
    iStorage->write_setting_i(sesskey, "RFCEnviron", conf_get_int(conf, CONF_rfc_environ));
    iStorage->write_setting_i(sesskey, "PassiveTelnet", conf_get_int(conf, CONF_passive_telnet));
    iStorage->write_setting_i(sesskey, "BackspaceIsDelete", conf_get_int(conf, CONF_bksp_is_delete));
    iStorage->write_setting_i(sesskey, "RXVTHomeEnd", conf_get_int(conf, CONF_rxvt_homeend));
    iStorage->write_setting_i(sesskey, "LinuxFunctionKeys", conf_get_int(conf, CONF_funky_type));
    iStorage->write_setting_i(sesskey, "NoApplicationKeys", conf_get_int(conf, CONF_no_applic_k));
    iStorage->write_setting_i(sesskey, "NoApplicationCursors", conf_get_int(conf, CONF_no_applic_c));
    iStorage->write_setting_i(sesskey, "NoMouseReporting", conf_get_int(conf, CONF_no_mouse_rep));
    iStorage->write_setting_i(sesskey, "NoRemoteResize", conf_get_int(conf, CONF_no_remote_resize));
    iStorage->write_setting_i(sesskey, "NoAltScreen", conf_get_int(conf, CONF_no_alt_screen));
    iStorage->write_setting_i(sesskey, "NoRemoteWinTitle", conf_get_int(conf, CONF_no_remote_wintitle));
    iStorage->write_setting_i(sesskey, "RemoteQTitleAction", conf_get_int(conf, CONF_remote_qtitle_action));
    iStorage->write_setting_i(sesskey, "NoDBackspace", conf_get_int(conf, CONF_no_dbackspace));
    iStorage->write_setting_i(sesskey, "NoRemoteCharset", conf_get_int(conf, CONF_no_remote_charset));
    iStorage->write_setting_i(sesskey, "ApplicationCursorKeys", conf_get_int(conf, CONF_app_cursor));
    iStorage->write_setting_i(sesskey, "ApplicationKeypad", conf_get_int(conf, CONF_app_keypad));
    iStorage->write_setting_i(sesskey, "NetHackKeypad", conf_get_int(conf, CONF_nethack_keypad));
    iStorage->write_setting_i(sesskey, "AltF4", conf_get_int(conf, CONF_alt_f4));
    iStorage->write_setting_i(sesskey, "AltSpace", conf_get_int(conf, CONF_alt_space));
    iStorage->write_setting_i(sesskey, "AltOnly", conf_get_int(conf, CONF_alt_only));
    iStorage->write_setting_i(sesskey, "ComposeKey", conf_get_int(conf, CONF_compose_key));
    iStorage->write_setting_i(sesskey, "CtrlAltKeys", conf_get_int(conf, CONF_ctrlaltkeys));
#ifdef OSX_META_KEY_CONFIG
    iStorage->write_setting_i(sesskey, "OSXOptionMeta", conf_get_int(conf, CONF_osx_option_meta));
    iStorage->write_setting_i(sesskey, "OSXCommandMeta", conf_get_int(conf, CONF_osx_command_meta));
#endif
    iStorage->write_setting_i(sesskey, "TelnetKey", conf_get_int(conf, CONF_telnet_keyboard));
    iStorage->write_setting_i(sesskey, "TelnetRet", conf_get_int(conf, CONF_telnet_newline));
    iStorage->write_setting_i(sesskey, "LocalEcho", conf_get_int(conf, CONF_localecho));
    iStorage->write_setting_i(sesskey, "LocalEdit", conf_get_int(conf, CONF_localedit));
    iStorage->write_setting_s(sesskey, "Answerback", conf_get_str(conf, CONF_answerback));
    iStorage->write_setting_i(sesskey, "AlwaysOnTop", conf_get_int(conf, CONF_alwaysontop));
    iStorage->write_setting_i(sesskey, "FullScreenOnAltEnter", conf_get_int(conf, CONF_fullscreenonaltenter));
    iStorage->write_setting_i(sesskey, "HideMousePtr", conf_get_int(conf, CONF_hide_mouseptr));
    iStorage->write_setting_i(sesskey, "SunkenEdge", conf_get_int(conf, CONF_sunken_edge));
    iStorage->write_setting_i(sesskey, "WindowBorder", conf_get_int(conf, CONF_window_border));
    iStorage->write_setting_i(sesskey, "CurType", conf_get_int(conf, CONF_cursor_type));
    iStorage->write_setting_i(sesskey, "BlinkCur", conf_get_int(conf, CONF_blink_cur));
    iStorage->write_setting_i(sesskey, "Beep", conf_get_int(conf, CONF_beep));
    iStorage->write_setting_i(sesskey, "BeepInd", conf_get_int(conf, CONF_beep_ind));
    iStorage->write_setting_filename(sesskey, "BellWaveFile", conf_get_filename(conf, CONF_bell_wavefile));
    iStorage->write_setting_i(sesskey, "BellOverload", conf_get_int(conf, CONF_bellovl));
    iStorage->write_setting_i(sesskey, "BellOverloadN", conf_get_int(conf, CONF_bellovl_n));
    iStorage->write_setting_i(sesskey, "BellOverloadT", conf_get_int(conf, CONF_bellovl_t)
#ifdef PUTTY_UNIX_H
		    * 1000
#endif
		    );
    iStorage->write_setting_i(sesskey, "BellOverloadS", conf_get_int(conf, CONF_bellovl_s)
#ifdef PUTTY_UNIX_H
		    * 1000
#endif
		    );
    iStorage->write_setting_i(sesskey, "ScrollbackLines", conf_get_int(conf, CONF_savelines));
    iStorage->write_setting_i(sesskey, "DECOriginMode", conf_get_int(conf, CONF_dec_om));
    iStorage->write_setting_i(sesskey, "AutoWrapMode", conf_get_int(conf, CONF_wrap_mode));
    iStorage->write_setting_i(sesskey, "LFImpliesCR", conf_get_int(conf, CONF_lfhascr));
    iStorage->write_setting_i(sesskey, "CRImpliesLF", conf_get_int(conf, CONF_crhaslf));
    iStorage->write_setting_i(sesskey, "DisableArabicShaping", conf_get_int(conf, CONF_arabicshaping));
    iStorage->write_setting_i(sesskey, "DisableBidi", conf_get_int(conf, CONF_bidi));
    iStorage->write_setting_i(sesskey, "WinNameAlways", conf_get_int(conf, CONF_win_name_always));
    iStorage->write_setting_s(sesskey, "WinTitle", conf_get_str(conf, CONF_wintitle));
    iStorage->write_setting_i(sesskey, "TermWidth", conf_get_int(conf, CONF_width));
    iStorage->write_setting_i(sesskey, "TermHeight", conf_get_int(conf, CONF_height));
    iStorage->write_setting_fontspec(sesskey, "Font", conf_get_fontspec(conf, CONF_font));
    iStorage->write_setting_i(sesskey, "FontQuality", conf_get_int(conf, CONF_font_quality));
    iStorage->write_setting_i(sesskey, "FontVTMode", conf_get_int(conf, CONF_vtmode));
    iStorage->write_setting_i(sesskey, "UseSystemColours", conf_get_int(conf, CONF_system_colour));
    iStorage->write_setting_i(sesskey, "TryPalette", conf_get_int(conf, CONF_try_palette));
    iStorage->write_setting_i(sesskey, "ANSIColour", conf_get_int(conf, CONF_ansi_colour));
    iStorage->write_setting_i(sesskey, "Xterm256Colour", conf_get_int(conf, CONF_xterm_256_colour));
    iStorage->write_setting_i(sesskey, "BoldAsColour", conf_get_int(conf, CONF_bold_style)-1);

    for (i = 0; i < 22; i++) {
	char buf[20], buf2[30];
	sprintf(buf, "Colour%d", i);
	sprintf(buf2, "%d,%d,%d",
		conf_get_int_int(conf, CONF_colours, i*3+0),
		conf_get_int_int(conf, CONF_colours, i*3+1),
		conf_get_int_int(conf, CONF_colours, i*3+2));
		iStorage->write_setting_s(sesskey, buf, buf2);
    }
    iStorage->write_setting_i(sesskey, "RawCNP", conf_get_int(conf, CONF_rawcnp));
    iStorage->write_setting_i(sesskey, "PasteRTF", conf_get_int(conf, CONF_rtf_paste));
    iStorage->write_setting_i(sesskey, "MouseIsXterm", conf_get_int(conf, CONF_mouse_is_xterm));
    iStorage->write_setting_i(sesskey, "RectSelect", conf_get_int(conf, CONF_rect_select));
    iStorage->write_setting_i(sesskey, "MouseOverride", conf_get_int(conf, CONF_mouse_override));
    for (i = 0; i < 256; i += 32) {
		char buf[20], buf2[256];
		int j;
		sprintf(buf, "Wordness%d", i);
		*buf2 = '\0';
		for (j = i; j < i + 32; j++) {
			sprintf(buf2 + strlen(buf2), "%s%d",
				(*buf2 ? "," : ""),
				conf_get_int_int(conf, CONF_wordness, j));
		}
		iStorage->write_setting_s(sesskey, buf, buf2);
    }
    iStorage->write_setting_s(sesskey, "LineCodePage", conf_get_str(conf, CONF_line_codepage));
    iStorage->write_setting_i(sesskey, "CJKAmbigWide", conf_get_int(conf, CONF_cjk_ambig_wide));
    iStorage->write_setting_i(sesskey, "UTF8Override", conf_get_int(conf, CONF_utf8_override));
    iStorage->write_setting_s(sesskey, "Printer", conf_get_str(conf, CONF_printer));
    iStorage->write_setting_i(sesskey, "CapsLockCyr", conf_get_int(conf, CONF_xlat_capslockcyr));
    iStorage->write_setting_i(sesskey, "ScrollBar", conf_get_int(conf, CONF_scrollbar));
    iStorage->write_setting_i(sesskey, "ScrollBarFullScreen", conf_get_int(conf, CONF_scrollbar_in_fullscreen));
    iStorage->write_setting_i(sesskey, "ScrollOnKey", conf_get_int(conf, CONF_scroll_on_key));
    iStorage->write_setting_i(sesskey, "ScrollOnDisp", conf_get_int(conf, CONF_scroll_on_disp));
    iStorage->write_setting_i(sesskey, "EraseToScrollback", conf_get_int(conf, CONF_erase_to_scrollback));
    iStorage->write_setting_i(sesskey, "LockSize", conf_get_int(conf, CONF_resize_action));
    iStorage->write_setting_i(sesskey, "BCE", conf_get_int(conf, CONF_bce));
    iStorage->write_setting_i(sesskey, "BlinkText", conf_get_int(conf, CONF_blinktext));
    iStorage->write_setting_i(sesskey, "X11Forward", conf_get_int(conf, CONF_x11_forward));
    iStorage->write_setting_s(sesskey, "X11Display", conf_get_str(conf, CONF_x11_display));
    iStorage->write_setting_i(sesskey, "X11AuthType", conf_get_int(conf, CONF_x11_auth));
    iStorage->write_setting_filename(sesskey, "X11AuthFile", conf_get_filename(conf, CONF_xauthfile));
    iStorage->write_setting_i(sesskey, "LocalPortAcceptAll", conf_get_int(conf, CONF_lport_acceptall));
    iStorage->write_setting_i(sesskey, "RemotePortAcceptAll", conf_get_int(conf, CONF_rport_acceptall));
    wmap(iStorage, sesskey, "PortForwardings", conf, CONF_portfwd, TRUE);
    iStorage->write_setting_i(sesskey, "BugIgnore1", 2-conf_get_int(conf, CONF_sshbug_ignore1));
    iStorage->write_setting_i(sesskey, "BugPlainPW1", 2-conf_get_int(conf, CONF_sshbug_plainpw1));
    iStorage->write_setting_i(sesskey, "BugRSA1", 2-conf_get_int(conf, CONF_sshbug_rsa1));
    iStorage->write_setting_i(sesskey, "BugIgnore2", 2-conf_get_int(conf, CONF_sshbug_ignore2));
    iStorage->write_setting_i(sesskey, "BugHMAC2", 2-conf_get_int(conf, CONF_sshbug_hmac2));
    iStorage->write_setting_i(sesskey, "BugDeriveKey2", 2-conf_get_int(conf, CONF_sshbug_derivekey2));
    iStorage->write_setting_i(sesskey, "BugRSAPad2", 2-conf_get_int(conf, CONF_sshbug_rsapad2));
    iStorage->write_setting_i(sesskey, "BugPKSessID2", 2-conf_get_int(conf, CONF_sshbug_pksessid2));
    iStorage->write_setting_i(sesskey, "BugRekey2", 2-conf_get_int(conf, CONF_sshbug_rekey2));
    iStorage->write_setting_i(sesskey, "BugMaxPkt2", 2-conf_get_int(conf, CONF_sshbug_maxpkt2));
    iStorage->write_setting_i(sesskey, "BugOldGex2", 2-conf_get_int(conf, CONF_sshbug_oldgex2));
    iStorage->write_setting_i(sesskey, "BugWinadj", 2-conf_get_int(conf, CONF_sshbug_winadj));
    iStorage->write_setting_i(sesskey, "BugChanReq", 2-conf_get_int(conf, CONF_sshbug_chanreq));
    iStorage->write_setting_i(sesskey, "StampUtmp", conf_get_int(conf, CONF_stamp_utmp));
    iStorage->write_setting_i(sesskey, "LoginShell", conf_get_int(conf, CONF_login_shell));
    iStorage->write_setting_i(sesskey, "ScrollbarOnLeft", conf_get_int(conf, CONF_scrollbar_on_left));
    iStorage->write_setting_fontspec(sesskey, "BoldFont", conf_get_fontspec(conf, CONF_boldfont));
    iStorage->write_setting_fontspec(sesskey, "WideFont", conf_get_fontspec(conf, CONF_widefont));
    iStorage->write_setting_fontspec(sesskey, "WideBoldFont", conf_get_fontspec(conf, CONF_wideboldfont));
    iStorage->write_setting_i(sesskey, "ShadowBold", conf_get_int(conf, CONF_shadowbold));
    iStorage->write_setting_i(sesskey, "ShadowBoldOffset", conf_get_int(conf, CONF_shadowboldoffset));
    iStorage->write_setting_s(sesskey, "SerialLine", conf_get_str(conf, CONF_serline));
    iStorage->write_setting_i(sesskey, "SerialSpeed", conf_get_int(conf, CONF_serspeed));
    iStorage->write_setting_i(sesskey, "SerialDataBits", conf_get_int(conf, CONF_serdatabits));
    iStorage->write_setting_i(sesskey, "SerialStopHalfbits", conf_get_int(conf, CONF_serstopbits));
    iStorage->write_setting_i(sesskey, "SerialParity", conf_get_int(conf, CONF_serparity));
    iStorage->write_setting_i(sesskey, "SerialFlowControl", conf_get_int(conf, CONF_serflow));
    iStorage->write_setting_s(sesskey, "WindowClass", conf_get_str(conf, CONF_winclass));
    iStorage->write_setting_i(sesskey, "ConnectionSharing", conf_get_int(conf, CONF_ssh_connection_sharing));
    iStorage->write_setting_i(sesskey, "ConnectionSharingUpstream", conf_get_int(conf, CONF_ssh_connection_sharing_upstream));
    iStorage->write_setting_i(sesskey, "ConnectionSharingDownstream", conf_get_int(conf, CONF_ssh_connection_sharing_downstream));
    wmap(iStorage, sesskey, "SSHManualHostKeys", conf, CONF_ssh_manual_hostkeys, FALSE);

	iStorage->write_setting_i(sesskey, "NoRemoteTabName", conf_get_int(conf, CONF_no_remote_tabname));
	iStorage->write_setting_i(sesskey, "NoRemoteTabNameInIcon", conf_get_int(conf, CONF_no_remote_tabname_in_icon));
    iStorage->write_setting_i(sesskey, "LinesAtAScroll", conf_get_int(conf, CONF_scrolllines));
	for (i = 0; i < AUTOCMD_COUNT; i++){
		char buf[64];
		int autocmd_enable = 0;
		bool exist = conf_try_get_int_int(conf, CONF_autocmd_enable, i, autocmd_enable);
		if (!exist){ 
			iStorage->write_setting_i(sesskey, "AutocmdCount", i);
			break; 
		}

		sprintf(buf, "AutocmdEnable%d", i);
		iStorage->write_setting_i(sesskey, buf, autocmd_enable);
	    sprintf(buf, "AutocmdExpect%d", i);
        iStorage->write_setting_s(sesskey, buf, conf_get_int_str(conf, CONF_expect, i));
		
		if (conf_get_int_int(conf, CONF_autocmd_hide,i)){
			char encryptStr[128] = {0};
			if (DES_Encrypt2Char(conf_get_int_str(conf, CONF_autocmd, i) ,PSWD, encryptStr)){
				conf_set_int_int(conf, CONF_autocmd_encrypted, i, 1);
				sprintf(buf, "Autocmd%d", i);
				iStorage->write_setting_s(sesskey, buf, encryptStr);
				sprintf(buf, "AutocmdEncrypted%d", i);
				iStorage->write_setting_i(sesskey, buf, 1);
			}else{
				sprintf(buf, "Autocmd%d", i);
		        iStorage->write_setting_s(sesskey, buf, conf_get_int_str(conf, CONF_autocmd, i));
				conf_set_int_int(conf, CONF_autocmd_encrypted, i, 0);
				sprintf(buf, "AutocmdEncrypted%d", i);
				iStorage->write_setting_i(sesskey, buf, 0);
			}
		}else{
			conf_set_int_int(conf, CONF_autocmd_encrypted, i, 0);
			sprintf(buf, "Autocmd%d", i);
			iStorage->write_setting_s(sesskey, buf, conf_get_int_str(conf, CONF_autocmd, i));
			sprintf(buf, "AutocmdEncrypted%d", i);
			iStorage->write_setting_i(sesskey, buf, 0);
		}
		
        sprintf(buf, "AutocmdHide%d", i);
        iStorage->write_setting_i(sesskey, buf, conf_get_int_int(conf, CONF_autocmd_hide, i));
    }

	iStorage->write_setting_s(sesskey, "AdbConStr", conf_get_str(conf, CONF_adb_con_str));
	iStorage->write_setting_s(sesskey, "AdbCmdStr", conf_get_str(conf, CONF_adb_cmd_str));
	iStorage->write_setting_i(sesskey, "AdbDevScanInterval", conf_get_int(conf, CONF_adb_dev_scan_interval));
	iStorage->write_setting_i(sesskey, "AdbCompelCRLF", conf_get_int(conf, CONF_adb_compel_crlf));

	iStorage->write_setting_i(sesskey, "DataVersion", conf_get_int(conf, CONF_data_version));
	iStorage->write_setting_i(sesskey, "GroupCollapse", conf_get_int(conf, CONF_group_collapse));
}

void load_settings_from_mem(const char *section, Conf *conf, const char* content)
{
	MemStore store;
	store.input(content);
	void *sesskey = store.open_settings_r(section);
	load_open_settings(&store, sesskey, conf);
	store.close_settings_r(sesskey);
}

bool check_load_mem_settings(const char *section, Conf *conf)
{
	bool ret = FALSE;
	if (!strcmp(section, LOCAL_SSH_SESSION_NAME))
	{
		load_settings_from_mem(section, conf,
			"Present=1\n"
			"HostName=127.0.0.1\n"
			"BackspaceIsDelete=0\n"
			"LFImpliesCR=1\n"
			"TermWidth=237\n"
			"TermHeight=63\n"
			"FontHeight=14\n"
			"LineCodePage=UTF-8\n"
			"AutocmdEnable0=1\n"
			"Autocmd0=EnterYourWindowsAccountHere\n"
			"AutocmdEnable1=1\n"
			"Autocmd1=EnterYourPasswordHere\n"
			"AutocmdHide1=0\n"
			"AutocmdCount=2\n"
			"TerminalType=xterm\n"
			);
		ret = TRUE;
	}
	else if (!strcmp(section, START_LOCAL_SSH_SERVER_NAME))
	{
		load_settings_from_mem(section, conf,
			"Present=1\n"
			"Protocol=adb\n"
			"PortNumber=0\n"
			"CloseOnExit=1\n"
			"BackspaceIsDelete=0\n"
			"LFImpliesCR=1\n"
			"TermWidth=96\n"
			"TermHeight=32\n"
			"LineCodePage=UTF-8\n"
			"AutocmdEnable0=1\n"
			"AutocmdExpect0=$\n"
			"Autocmd0=export TERM=xterm\n"
			"AutocmdEnable1=1\n"
			"AutocmdExpect1=$\n"
			"Autocmd1=cat>start_localhost_sshserver.sh<<END_OF_CMD\n"
			"AutocmdHide1=0\n"
			"AutocmdEnable2=1\n"
			"AutocmdExpect2=>\n"
			"Autocmd2=(grep -q sshd /etc/passwd)||bash -c '(echo \"sshd:x:74:74:Privilege-separated SSH:/var/empty/sshd:/sbin/nologin\" >>/etc/passwd)'\n"
			"AutocmdEnable3=1\n"
			"AutocmdExpect3=>\n"
			"Autocmd3=[ -f /etc/ssh/ssh_host_dsa_key ] || (ssh-keygen -t dsa -f /etc/ssh/ssh_host_dsa_key -N '' )\n"
			"AutocmdEnable4=1\n"
			"AutocmdExpect4=>\n"
			"Autocmd4=[ -f /etc/ssh/ssh_host_rsa_key ] || (ssh-keygen -t rsa -f /etc/ssh/ssh_host_rsa_key -N '' )\n"
			"AutocmdEnable5=1\n"
			"AutocmdExpect5=>\n"
			"Autocmd5=[ -f /etc/ssh/ssh_host_ecdsa_key ] || (ssh-keygen -t ecdsa -f /etc/ssh/ssh_host_ecdsa_key -N '' )\n"
			"AutocmdEnable6=1\n"
			"AutocmdExpect6=>\n"
			"Autocmd6=[ -f /etc/ssh/ssh_host_ed25519_key ] || (ssh-keygen -t ed25519 -f /etc/ssh/ssh_host_ed25519_key -N '' )\n"
			"AutocmdEnable7=1\n"
			"AutocmdExpect7=>\n"
			"Autocmd7=mkdir -p /var/empty/\n"
			"AutocmdEnable8=1\n"
			"AutocmdExpect8=>\n"
			"Autocmd8=/usr/bin/sshd &\n"
			"AutocmdEnable9=1\n"
			"AutocmdExpect9=>\n"
			"Autocmd9=END_OF_CMD\n"
			"AutocmdEnable10=1\n"
			"AutocmdExpect10=$\n"
			"Autocmd10=powershell Start-Process -Verb runas -ArgumentList ' start_localhost_sshserver.sh' bash <<END_OF_PS\n"
			"AutocmdEnable11=1\n"
			"AutocmdExpect11=>\n"
			"Autocmd11=END_OF_PS\n"
			"AutocmdCount=12\n"
			"AdbConStr= \n"
			"AdbCmdStr=C:\\Program Files\\Git\\bin\\bash --posix -i\n"
			"AdbCompelCRLF=0\n"
			"TerminalType=xterm\n"
			);
		ret = TRUE;
	}
	if (ret){
		conf_set_str(conf, CONF_session_name, section);
		if (conf_launchable(conf))
		{
			extern void add_to_jumplist_in_bg(const char* sessionname);
			add_to_jumplist_in_bg(section);
		}
	}
	return ret;
}

void load_settings(const char *section, Conf *conf, IStore* iStorage)
{
	if (section && check_load_mem_settings(section, conf)) { return; }

    void *sesskey;	
	TmplStore tmpl_store(gStorage);
	IStore* storageInterface = iStorage ? iStorage : &tmpl_store;

    sesskey = storageInterface->open_settings_r(section);
    load_open_settings(storageInterface, sesskey, conf);
    storageInterface->close_settings_r(sesskey);

#ifdef _WINDOWS
	char default_log_path[512] = {0};
	WinRegStore::open_read_settings_s(
		"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders", 
		"Desktop",
		default_log_path, 
		sizeof(default_log_path));
	conf_set_str(conf, CONF_default_log_path, default_log_path);
#else
	conf_set_str(conf, CONF_default_log_path, "~/");
#endif

	if (!section || !*section)
		conf_set_str(conf, CONF_session_name, DEFAULT_SESSION_NAME);
	else 
		conf_set_str(conf, CONF_session_name, section);

	if (conf_launchable(conf))
	{
		extern void add_to_jumplist_in_bg(const char* sessionname);
		add_to_jumplist_in_bg(section);
	}
}

void load_open_settings(IStore* iStorage, void *sesskey, Conf *conf)
{
    int i;
    char *prot;

	conf_clear(conf);
    conf_set_int(conf, CONF_ssh_subsys, 0);   /* FIXME: load this properly */
    conf_set_str(conf, CONF_remote_cmd, "");
    conf_set_str(conf, CONF_remote_cmd2, "");
    conf_set_str(conf, CONF_ssh_nc_host, "");
    conf_set_str(conf, CONF_password, "");
	conf_set_int(conf, CONF_autocmd_index, 0);
	conf_set_int(conf, CONF_autocmd_try, 0);
	conf_set_int(conf, CONF_autocmd_last_lineno, 0);

    gpps(iStorage, sesskey, "HostName", "", conf, CONF_host);
    gppfile(iStorage, sesskey, "LogFileName", conf, CONF_logfilename);
    gppi(iStorage, sesskey, "LogType", 0, conf, CONF_logtype);
    gppi(iStorage, sesskey, "LogFileClash", LGXF_ASK, conf, CONF_logxfovr);
    gppi(iStorage, sesskey, "LogFlush", 1, conf, CONF_logflush);
    gppi(iStorage, sesskey, "SSHLogOmitPasswords", 1, conf, CONF_logomitpass);
    gppi(iStorage, sesskey, "SSHLogOmitData", 0, conf, CONF_logomitdata);

    prot = gpps_raw(iStorage, sesskey, "Protocol", "ssh");
    conf_set_int(conf, CONF_protocol, default_protocol);
    conf_set_int(conf, CONF_port, default_port);
    {
	const Backend *b = backend_from_name(prot);
	if (b) {
	    conf_set_int(conf, CONF_protocol, b->protocol);
	    gppi(iStorage, sesskey, "PortNumber", default_port, conf, CONF_port);
	}
    }
    sfree(prot);

    /* Address family selection */
    gppi(iStorage, sesskey, "AddressFamily", ADDRTYPE_UNSPEC, conf, CONF_addressfamily);

    /* The CloseOnExit numbers are arranged in a different order from
     * the standard FORCE_ON / FORCE_OFF / AUTO. */
	i = gppi_raw(iStorage, sesskey, "CloseOnExit", 0); 
	conf_set_int(conf, CONF_close_on_exit, (i + 1) % 3);
    gppi(iStorage, sesskey, "WarnOnClose", 1, conf, CONF_warn_on_close);
    {
	/* This is two values for backward compatibility with 0.50/0.51 */
	int pingmin, pingsec;
	pingmin = gppi_raw(iStorage, sesskey, "PingInterval", 0);
	pingsec = gppi_raw(iStorage, sesskey, "PingIntervalSecs", 3);
	conf_set_int(conf, CONF_ping_interval, pingmin * 60 + pingsec);
    }
    gppi(iStorage, sesskey, "TCPNoDelay", 1, conf, CONF_tcp_nodelay);
    gppi(iStorage, sesskey, "TCPKeepalives", 0, conf, CONF_tcp_keepalives);
    gpps(iStorage, sesskey, "TerminalType", "xterm", conf, CONF_termtype);
    gpps(iStorage, sesskey, "TerminalSpeed", "38400,38400", conf, CONF_termspeed);
    if (!gppmap(iStorage, sesskey, "TerminalModes", conf, CONF_ttymodes)) {
	/* This hardcodes a big set of defaults in any new saved
	 * sessions. Let's hope we don't change our mind. */
	for (i = 0; ttymodes[i]; i++)
	    conf_set_str_str(conf, CONF_ttymodes, ttymodes[i], "A");
    }

    /* proxy settings */
    gpps(iStorage, sesskey, "ProxyExcludeList", "", conf, CONF_proxy_exclude_list);
    i = gppi_raw(iStorage, sesskey, "ProxyDNS", 1); conf_set_int(conf, CONF_proxy_dns, (i+1)%3);
    gppi(iStorage, sesskey, "ProxyLocalhost", 0, conf, CONF_even_proxy_localhost);
    gppi(iStorage, sesskey, "ProxyMethod", -1, conf, CONF_proxy_type);
    if (conf_get_int(conf, CONF_proxy_type) == -1) {
        int i;
        i = gppi_raw(iStorage, sesskey, "ProxyType", 0);
        if (i == 0)
            conf_set_int(conf, CONF_proxy_type, PROXY_NONE);
        else if (i == 1)
            conf_set_int(conf, CONF_proxy_type, PROXY_HTTP);
        else if (i == 3)
            conf_set_int(conf, CONF_proxy_type, PROXY_TELNET);
        else if (i == 4)
            conf_set_int(conf, CONF_proxy_type, PROXY_CMD);
        else {
            i = gppi_raw(iStorage, sesskey, "ProxySOCKSVersion", 5);
            if (i == 5)
                conf_set_int(conf, CONF_proxy_type, PROXY_SOCKS5);
            else
                conf_set_int(conf, CONF_proxy_type, PROXY_SOCKS4);
        }
    }
    gpps(iStorage, sesskey, "ProxyHost", "proxy", conf, CONF_proxy_host);
    gppi(iStorage, sesskey, "ProxyPort", 80, conf, CONF_proxy_port);
    gpps(iStorage, sesskey, "ProxyUsername", "", conf, CONF_proxy_username);
    gpps(iStorage, sesskey, "ProxyPassword", "", conf, CONF_proxy_password);
    gpps(iStorage, sesskey, "ProxyTelnetCommand", "connect %host %port\\n",
	 conf, CONF_proxy_telnet_command);
    gppmap(iStorage, sesskey, "Environment", conf, CONF_environmt);
    gpps(iStorage, sesskey, "UserName", "", conf, CONF_username);
    gppi(iStorage, sesskey, "UserNameFromEnvironment", 0, conf, CONF_username_from_env);
    gpps(iStorage, sesskey, "LocalUserName", "", conf, CONF_localusername);
    gppi(iStorage, sesskey, "NoPTY", 0, conf, CONF_nopty);
    gppi(iStorage, sesskey, "Compression", 0, conf, CONF_compression);
    gppi(iStorage, sesskey, "TryAgent", 1, conf, CONF_tryagent);
    gppi(iStorage, sesskey, "AgentFwd", 0, conf, CONF_agentfwd);
    gppi(iStorage, sesskey, "ChangeUsername", 0, conf, CONF_change_username);
    gppi(iStorage, sesskey, "GssapiFwd", 0, conf, CONF_gssapifwd);
    gprefs(iStorage, sesskey, "Cipher", "\0",
	   ciphernames, CIPHER_MAX, conf, CONF_ssh_cipherlist);
    {
	/* Backward-compatibility: we used to have an option to
	 * disable gex under the "bugs" panel after one report of
	 * a server which offered it then choked, but we never got
	 * a server version string or any other reports. */
	const char *default_kexes;
	i = 2 - gppi_raw(iStorage, sesskey, "BugDHGEx2", 0);
	if (i == FORCE_ON)
            default_kexes = "ecdh,dh-group14-sha1,dh-group1-sha1,rsa,"
                "WARN,dh-gex-sha1";
	else
            default_kexes = "ecdh,dh-gex-sha1,dh-group14-sha1,"
                "dh-group1-sha1,rsa,WARN";
	gprefs(iStorage, sesskey, "KEX", default_kexes,
	       kexnames, KEX_MAX, conf, CONF_ssh_kexlist);
    }
    gppi(iStorage, sesskey, "RekeyTime", 60, conf, CONF_ssh_rekey_time);
    gpps(iStorage, sesskey, "RekeyBytes", "1G", conf, CONF_ssh_rekey_data);
    /* SSH-2 only by default */
    gppi(iStorage, sesskey, "SshProt", 3, conf, CONF_sshprot);
    gpps(iStorage, sesskey, "LogHost", "", conf, CONF_loghost);
    gppi(iStorage, sesskey, "SSH2DES", 0, conf, CONF_ssh2_des_cbc);
    gppi(iStorage, sesskey, "SshNoAuth", 0, conf, CONF_ssh_no_userauth);
    gppi(iStorage, sesskey, "SshBanner", 1, conf, CONF_ssh_show_banner);
    gppi(iStorage, sesskey, "AuthTIS", 0, conf, CONF_try_tis_auth);
    gppi(iStorage, sesskey, "AuthKI", 1, conf, CONF_try_ki_auth);
    gppi(iStorage, sesskey, "AuthGSSAPI", 1, conf, CONF_try_gssapi_auth);
#ifndef NO_GSSAPI
    gprefs(iStorage, sesskey, "GSSLibs", "\0",
	   gsslibkeywords, ngsslibs, conf, CONF_ssh_gsslist);
    gppfile(iStorage, sesskey, "GSSCustom", conf, CONF_ssh_gss_custom);
#endif
    gppi(iStorage, sesskey, "SshNoShell", 0, conf, CONF_ssh_no_shell);
    gppfile(iStorage, sesskey, "PublicKeyFile", conf, CONF_keyfile);
    gpps(iStorage, sesskey, "RemoteCommand", "", conf, CONF_remote_cmd);
    gppi(iStorage, sesskey, "RFCEnviron", 0, conf, CONF_rfc_environ);
    gppi(iStorage, sesskey, "PassiveTelnet", 0, conf, CONF_passive_telnet);
    gppi(iStorage, sesskey, "BackspaceIsDelete", 1, conf, CONF_bksp_is_delete);
    gppi(iStorage, sesskey, "RXVTHomeEnd", 0, conf, CONF_rxvt_homeend);
    gppi(iStorage, sesskey, "LinuxFunctionKeys", 0, conf, CONF_funky_type);
    gppi(iStorage, sesskey, "NoApplicationKeys", 0, conf, CONF_no_applic_k);
    gppi(iStorage, sesskey, "NoApplicationCursors", 0, conf, CONF_no_applic_c);
    gppi(iStorage, sesskey, "NoMouseReporting", 0, conf, CONF_no_mouse_rep);
    gppi(iStorage, sesskey, "NoRemoteResize", 0, conf, CONF_no_remote_resize);
    gppi(iStorage, sesskey, "NoAltScreen", 0, conf, CONF_no_alt_screen);
    gppi(iStorage, sesskey, "NoRemoteWinTitle", 0, conf, CONF_no_remote_wintitle);
    {
	/* Backward compatibility */
	int no_remote_qtitle = gppi_raw(iStorage, sesskey, "NoRemoteQTitle", 1);
	/* We deliberately interpret the old setting of "no response" as
	 * "empty string". This changes the behaviour, but hopefully for
	 * the better; the user can always recover the old behaviour. */
	gppi(iStorage, sesskey, "RemoteQTitleAction",
	     no_remote_qtitle ? TITLE_EMPTY : TITLE_REAL,
	     conf, CONF_remote_qtitle_action);
    }
    gppi(iStorage, sesskey, "NoDBackspace", 0, conf, CONF_no_dbackspace);
    gppi(iStorage, sesskey, "NoRemoteCharset", 0, conf, CONF_no_remote_charset);
    gppi(iStorage, sesskey, "ApplicationCursorKeys", 0, conf, CONF_app_cursor);
    gppi(iStorage, sesskey, "ApplicationKeypad", 0, conf, CONF_app_keypad);
    gppi(iStorage, sesskey, "NetHackKeypad", 0, conf, CONF_nethack_keypad);
    gppi(iStorage, sesskey, "AltF4", 1, conf, CONF_alt_f4);
    gppi(iStorage, sesskey, "AltSpace", 0, conf, CONF_alt_space);
    gppi(iStorage, sesskey, "AltOnly", 0, conf, CONF_alt_only);
    gppi(iStorage, sesskey, "ComposeKey", 0, conf, CONF_compose_key);
    gppi(iStorage, sesskey, "CtrlAltKeys", 1, conf, CONF_ctrlaltkeys);
#ifdef OSX_META_KEY_CONFIG
    gppi(iStorage, sesskey, "OSXOptionMeta", 1, conf, CONF_osx_option_meta);
    gppi(iStorage, sesskey, "OSXCommandMeta", 0, conf, CONF_osx_command_meta);
#endif
    gppi(iStorage, sesskey, "TelnetKey", 0, conf, CONF_telnet_keyboard);
    gppi(iStorage, sesskey, "TelnetRet", 1, conf, CONF_telnet_newline);
    gppi(iStorage, sesskey, "LocalEcho", AUTO, conf, CONF_localecho);
    gppi(iStorage, sesskey, "LocalEdit", AUTO, conf, CONF_localedit);
    gpps(iStorage, sesskey, "Answerback", "PuTTY", conf, CONF_answerback);
    gppi(iStorage, sesskey, "AlwaysOnTop", 0, conf, CONF_alwaysontop);
    gppi(iStorage, sesskey, "FullScreenOnAltEnter", 0, conf, CONF_fullscreenonaltenter);
    gppi(iStorage, sesskey, "HideMousePtr", 0, conf, CONF_hide_mouseptr);
    gppi(iStorage, sesskey, "SunkenEdge", 0, conf, CONF_sunken_edge);
    gppi(iStorage, sesskey, "WindowBorder", 1, conf, CONF_window_border);
    gppi(iStorage, sesskey, "CurType", 0, conf, CONF_cursor_type);
    gppi(iStorage, sesskey, "BlinkCur", 0, conf, CONF_blink_cur);
    /* pedantic compiler tells me I can't use conf, CONF_beep as an int * :-) */
    gppi(iStorage, sesskey, "Beep", 1, conf, CONF_beep);
    gppi(iStorage, sesskey, "BeepInd", 0, conf, CONF_beep_ind);
    gppfile(iStorage, sesskey, "BellWaveFile", conf, CONF_bell_wavefile);
    gppi(iStorage, sesskey, "BellOverload", 1, conf, CONF_bellovl);
    gppi(iStorage, sesskey, "BellOverloadN", 5, conf, CONF_bellovl_n);
    i = gppi_raw(iStorage, sesskey, "BellOverloadT", 2*TICKSPERSEC
#ifdef PUTTY_UNIX_H
				   *1000
#endif
				   );
    conf_set_int(conf, CONF_bellovl_t, i
#ifdef PUTTY_UNIX_H
		 / 1000
#endif
		 );
    i = gppi_raw(iStorage, sesskey, "BellOverloadS", 5*TICKSPERSEC
#ifdef PUTTY_UNIX_H
				   *1000
#endif
				   );
    conf_set_int(conf, CONF_bellovl_s, i
#ifdef PUTTY_UNIX_H
		 / 1000
#endif
		 );
    gppi(iStorage, sesskey, "ScrollbackLines", 99999, conf, CONF_savelines);
    gppi(iStorage, sesskey, "DECOriginMode", 0, conf, CONF_dec_om);
    gppi(iStorage, sesskey, "AutoWrapMode", 1, conf, CONF_wrap_mode);
    gppi(iStorage, sesskey, "LFImpliesCR", 0, conf, CONF_lfhascr);
    gppi(iStorage, sesskey, "CRImpliesLF", 0, conf, CONF_crhaslf);
    gppi(iStorage, sesskey, "DisableArabicShaping", 0, conf, CONF_arabicshaping);
    gppi(iStorage, sesskey, "DisableBidi", 0, conf, CONF_bidi);
    gppi(iStorage, sesskey, "WinNameAlways", 1, conf, CONF_win_name_always);
    gpps(iStorage, sesskey, "WinTitle", "", conf, CONF_wintitle);
    gppi(iStorage, sesskey, "TermWidth", 80, conf, CONF_width);
    gppi(iStorage, sesskey, "TermHeight", 24, conf, CONF_height);
    gppfont(iStorage, sesskey, "Font", conf, CONF_font);
    gppi(iStorage, sesskey, "FontQuality", FQ_DEFAULT, conf, CONF_font_quality);
    gppi(iStorage, sesskey, "FontVTMode", VT_UNICODE, conf, CONF_vtmode);
    gppi(iStorage, sesskey, "UseSystemColours", 0, conf, CONF_system_colour);
    gppi(iStorage, sesskey, "TryPalette", 0, conf, CONF_try_palette);
    gppi(iStorage, sesskey, "ANSIColour", 1, conf, CONF_ansi_colour);
    gppi(iStorage, sesskey, "Xterm256Colour", 1, conf, CONF_xterm_256_colour);
    i = gppi_raw(iStorage, sesskey, "BoldAsColour", 1); conf_set_int(conf, CONF_bold_style, i+1);

    for (i = 0; i < 22; i++) {
	static const char *const defaults[] = {
	    "187,187,187", "255,255,255", "0,0,0", "85,85,85", "0,0,0",
	    "0,255,0", "0,0,0", "85,85,85", "187,0,0", "255,85,85",
	    "0,187,0", "85,255,85", "187,187,0", "255,255,85", "0,0,187",
	    "85,85,255", "187,0,187", "255,85,255", "0,187,187",
	    "85,255,255", "187,187,187", "255,255,255"
	};
	char buf[20], *buf2;
	int c0, c1, c2;
	sprintf(buf, "Colour%d", i);
	buf2 = gpps_raw(iStorage, sesskey, buf, defaults[i]);
	if (sscanf(buf2, "%d,%d,%d", &c0, &c1, &c2) == 3) {
	    conf_set_int_int(conf, CONF_colours, i*3+0, c0);
	    conf_set_int_int(conf, CONF_colours, i*3+1, c1);
	    conf_set_int_int(conf, CONF_colours, i*3+2, c2);
	}
	sfree(buf2);
    }
    gppi(iStorage, sesskey, "RawCNP", 0, conf, CONF_rawcnp);
    gppi(iStorage, sesskey, "PasteRTF", 0, conf, CONF_rtf_paste);
    gppi(iStorage, sesskey, "MouseIsXterm", 0, conf, CONF_mouse_is_xterm);
    gppi(iStorage, sesskey, "RectSelect", 0, conf, CONF_rect_select);
    gppi(iStorage, sesskey, "MouseOverride", 1, conf, CONF_mouse_override);
    for (i = 0; i < 256; i += 32) {
	static const char *const defaults[] = {
	    "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0",
	    "0,1,2,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1",
	    "1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,2",
	    "1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1",
	    "1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1",
	    "1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1",
	    "2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2",
	    "2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2"
	};
	char buf[20], *buf2, *p;
	int j;
	sprintf(buf, "Wordness%d", i);
	buf2 = gpps_raw(iStorage, sesskey, buf, defaults[i / 32]);
	p = buf2;
	for (j = i; j < i + 32; j++) {
	    char *q = p;
	    while (*p && *p != ',')
		p++;
	    if (*p == ',')
		*p++ = '\0';
	    conf_set_int_int(conf, CONF_wordness, j, atoi(q));
	}
	sfree(buf2);
    }
    /*
     * The empty default for LineCodePage will be converted later
     * into a plausible default for the locale.
     */
    gpps(iStorage, sesskey, "LineCodePage", "UTF-8", conf, CONF_line_codepage);
    gppi(iStorage, sesskey, "CJKAmbigWide", 0, conf, CONF_cjk_ambig_wide);
    gppi(iStorage, sesskey, "UTF8Override", 1, conf, CONF_utf8_override);
    gpps(iStorage, sesskey, "Printer", "", conf, CONF_printer);
    gppi(iStorage, sesskey, "CapsLockCyr", 0, conf, CONF_xlat_capslockcyr);
    gppi(iStorage, sesskey, "ScrollBar", 1, conf, CONF_scrollbar);
    gppi(iStorage, sesskey, "ScrollBarFullScreen", 0, conf, CONF_scrollbar_in_fullscreen);
    gppi(iStorage, sesskey, "ScrollOnKey", 0, conf, CONF_scroll_on_key);
    gppi(iStorage, sesskey, "ScrollOnDisp", 1, conf, CONF_scroll_on_disp);
    gppi(iStorage, sesskey, "EraseToScrollback", 1, conf, CONF_erase_to_scrollback);
    gppi(iStorage, sesskey, "LockSize", 0, conf, CONF_resize_action);
    gppi(iStorage, sesskey, "BCE", 1, conf, CONF_bce);
    gppi(iStorage, sesskey, "BlinkText", 0, conf, CONF_blinktext);
    gppi(iStorage, sesskey, "X11Forward", 1, conf, CONF_x11_forward);
    gpps(iStorage, sesskey, "X11Display", "localhost:0", conf, CONF_x11_display);
    gppi(iStorage, sesskey, "X11AuthType", X11_MIT, conf, CONF_x11_auth);
    gppfile(iStorage, sesskey, "X11AuthFile", conf, CONF_xauthfile);

    gppi(iStorage, sesskey, "LocalPortAcceptAll", 0, conf, CONF_lport_acceptall);
    gppi(iStorage, sesskey, "RemotePortAcceptAll", 0, conf, CONF_rport_acceptall);
    gppmap(iStorage, sesskey, "PortForwardings", conf, CONF_portfwd);
    i = gppi_raw(iStorage, sesskey, "BugIgnore1", 0); conf_set_int(conf, CONF_sshbug_ignore1, 2-i);
    i = gppi_raw(iStorage, sesskey, "BugPlainPW1", 0); conf_set_int(conf, CONF_sshbug_plainpw1, 2-i);
    i = gppi_raw(iStorage, sesskey, "BugRSA1", 0); conf_set_int(conf, CONF_sshbug_rsa1, 2-i);
    i = gppi_raw(iStorage, sesskey, "BugIgnore2", 0); conf_set_int(conf, CONF_sshbug_ignore2, 2-i);
    {
	int i;
	i = gppi_raw(iStorage, sesskey, "BugHMAC2", 0); conf_set_int(conf, CONF_sshbug_hmac2, 2-i);
	if (2-i == AUTO) {
	    i = gppi_raw(iStorage, sesskey, "BuggyMAC", 0);
	    if (i == 1)
		conf_set_int(conf, CONF_sshbug_hmac2, FORCE_ON);
	}
    }
    i = gppi_raw(iStorage, sesskey, "BugDeriveKey2", 0); conf_set_int(conf, CONF_sshbug_derivekey2, 2-i);
    i = gppi_raw(iStorage, sesskey, "BugRSAPad2", 0); conf_set_int(conf, CONF_sshbug_rsapad2, 2-i);
    i = gppi_raw(iStorage, sesskey, "BugPKSessID2", 0); conf_set_int(conf, CONF_sshbug_pksessid2, 2-i);
    i = gppi_raw(iStorage, sesskey, "BugRekey2", 0); conf_set_int(conf, CONF_sshbug_rekey2, 2-i);
    i = gppi_raw(iStorage, sesskey, "BugMaxPkt2", 0); conf_set_int(conf, CONF_sshbug_maxpkt2, 2-i);
    i = gppi_raw(iStorage, sesskey, "BugOldGex2", 0); conf_set_int(conf, CONF_sshbug_oldgex2, 2-i);
    i = gppi_raw(iStorage, sesskey, "BugWinadj", 0); conf_set_int(conf, CONF_sshbug_winadj, 2-i);
    i = gppi_raw(iStorage, sesskey, "BugChanReq", 0); conf_set_int(conf, CONF_sshbug_chanreq, 2-i);
    conf_set_int(conf, CONF_ssh_simple, FALSE);
    gppi(iStorage, sesskey, "StampUtmp", 1, conf, CONF_stamp_utmp);
    gppi(iStorage, sesskey, "LoginShell", 1, conf, CONF_login_shell);
    gppi(iStorage, sesskey, "ScrollbarOnLeft", 0, conf, CONF_scrollbar_on_left);
    gppi(iStorage, sesskey, "ShadowBold", 0, conf, CONF_shadowbold);
    gppfont(iStorage, sesskey, "BoldFont", conf, CONF_boldfont);
    gppfont(iStorage, sesskey, "WideFont", conf, CONF_widefont);
    gppfont(iStorage, sesskey, "WideBoldFont", conf, CONF_wideboldfont);
    gppi(iStorage, sesskey, "ShadowBoldOffset", 1, conf, CONF_shadowboldoffset);
    gpps(iStorage, sesskey, "SerialLine", "", conf, CONF_serline);
    gppi(iStorage, sesskey, "SerialSpeed", 9600, conf, CONF_serspeed);
    gppi(iStorage, sesskey, "SerialDataBits", 8, conf, CONF_serdatabits);
    gppi(iStorage, sesskey, "SerialStopHalfbits", 2, conf, CONF_serstopbits);
    gppi(iStorage, sesskey, "SerialParity", SER_PAR_NONE, conf, CONF_serparity);
    gppi(iStorage, sesskey, "SerialFlowControl", SER_FLOW_XONXOFF, conf, CONF_serflow);
    gpps(iStorage, sesskey, "WindowClass", "", conf, CONF_winclass);
    gppi(iStorage, sesskey, "ConnectionSharing", 0, conf, CONF_ssh_connection_sharing);
    gppi(iStorage, sesskey, "ConnectionSharingUpstream", 1, conf, CONF_ssh_connection_sharing_upstream);
    gppi(iStorage, sesskey, "ConnectionSharingDownstream", 1, conf, CONF_ssh_connection_sharing_downstream);
    gppmap(iStorage, sesskey, "SSHManualHostKeys", conf, CONF_ssh_manual_hostkeys);

	gppi(iStorage, sesskey, "NoRemoteTabName", 0, conf, CONF_no_remote_tabname);
	gppi(iStorage, sesskey, "NoRemoteTabNameInIcon", 1, conf, CONF_no_remote_tabname_in_icon);
	gppi(iStorage, sesskey, "LinesAtAScroll", 3, conf, CONF_scrolllines);
	int autocmd_count = gppi_raw(iStorage, sesskey, "AutocmdCount", AUTOCMD_COUNT);
	for (i = 0; i < autocmd_count; i++){
        char buf[64];
        sprintf(buf, "AutocmdEnable%d", i);
		int default_enable = i > 1 ? -1 : 1;

		int autocmd_enabled = gppi_raw(iStorage, sesskey, buf, default_enable);
		if (autocmd_enabled < 0){ break; }
		conf_set_int_int(conf, CONF_autocmd_enable, i, autocmd_enabled);	

	    sprintf(buf, "AutocmdExpect%d", i);
		char result[512] = {0};
        gpps_s(iStorage,  sesskey, buf, i==0?"ogin: "
                           :i==1?"assword: "
                           :"", 
						   conf, CONF_expect, i);
                           
        sprintf(buf, "AutocmdHide%d", i);
        gppi_i(iStorage,  sesskey, buf, i==1?1:0, conf, CONF_autocmd_hide, i);
		sprintf(buf, "AutocmdEncrypted%d", i);
        gppi_i(iStorage,  sesskey, buf, 0, conf, CONF_autocmd_encrypted, i);
        sprintf(buf, "Autocmd%d", i);
        gpps_s(iStorage,  sesskey, buf, "", conf, CONF_autocmd, i);

		if (conf_get_int_int(conf, CONF_autocmd_hide, i)){
			if (0 == conf_get_int_int(conf, CONF_autocmd_encrypted, i)){
				char encryptStr[128] = {0};
				if (DES_Encrypt2Char(conf_get_int_str(conf, CONF_autocmd, i),PSWD, encryptStr)){
					conf_set_int_int(conf, CONF_autocmd_encrypted, i, 1);
					sprintf(buf, "AutocmdEncrypted%d", i);
					iStorage->write_setting_i(sesskey, buf, conf_get_int_int(conf, CONF_autocmd_encrypted, i));
					sprintf(buf, "Autocmd%d", i);
					iStorage->write_setting_s(sesskey, buf, encryptStr);
				}
			}else{
				char plainStr[128] = {0};
				char* autocmd = conf_get_int_str( conf, CONF_autocmd, i);
				if (DES_DecryptFromChar(autocmd, strlen(autocmd), PSWD, plainStr)){
					conf_set_int_str(conf, CONF_autocmd, i, plainStr);
				}

			}
		}
        
    }

	gpps(iStorage, sesskey, "AdbConStr", "", conf, CONF_adb_con_str);
	gpps(iStorage, sesskey, "AdbCmdStr", "&padb.exe -s &1 shell", conf, CONF_adb_cmd_str);
	gppi(iStorage, sesskey, "AdbDevScanInterval", 0, conf, CONF_adb_dev_scan_interval);
	gppi(iStorage, sesskey, "AdbCompelCRLF", 1, conf, CONF_adb_compel_crlf);
	gppi(iStorage, sesskey, "DataVersion", 1, conf, CONF_data_version);
	gppi(iStorage, sesskey, "GroupCollapse", 1, conf, CONF_group_collapse);

	if (!isInited){
		DEFAULT_STR_VALUE["TerminalModes"] = "CS7=A,CS8=A,DISCARD=A,DSUSP=A,ECHO=A,ECHOCTL=A,ECHOE=A,ECHOK=A,ECHOKE=A,ECHONL=A,EOF=A,EOL=A,EOL2=A,ERASE=A,FLUSH=A,ICANON=A,ICRNL=A,IEXTEN=A,IGNCR=A,IGNPAR=A,IMAXBEL=A,INLCR=A,INPCK=A,INTR=A,ISIG=A,ISTRIP=A,IUCLC=A,IXANY=A,IXOFF=A,IXON=A,KILL=A,LNEXT=A,NOFLSH=A,OCRNL=A,OLCUC=A,ONLCR=A,ONLRET=A,ONOCR=A,OPOST=A,PARENB=A,PARMRK=A,PARODD=A,PENDIN=A,QUIT=A,REPRINT=A,START=A,STATUS=A,STOP=A,SUSP=A,SWTCH=A,TOSTOP=A,WERASE=A,XCASE=A";
		DEFAULT_INT_VALUE["ProxyMethod"] = 0;
		DEFAULT_STR_VALUE["Cipher"] = "chacha20,aes,blowfish,3des,WARN,arcfour,des";
		DEFAULT_STR_VALUE["GSSLibs"] = "gssapi32,sspi,custom";
		DEFAULT_STR_VALUE["FontName"] = "Courier New";
		DEFAULT_STR_VALUE["Font"] = "Courier New";
		DEFAULT_INT_VALUE["FontHeight"] = 10;
		DEFAULT_INT_VALUE["AutocmdCount"] = 2;
		DEFAULT_INT_VALUE["Present"] = 1;
		DEFAULT_STR_VALUE["SerialLine"] = "COM1";
		DEFAULT_STR_VALUE["LogFileName"] = "putty.log";
		DEFAULT_STR_VALUE["Environment"] = "";
		DEFAULT_STR_VALUE["PortForwardings"] = "";
		DEFAULT_STR_VALUE["SSHManualHostKeys"] = "";
		DEFAULT_STR_VALUE["Autocmd2"] = "";
		DEFAULT_STR_VALUE["Autocmd3"] = "";
		DEFAULT_STR_VALUE["Autocmd4"] = "";
		DEFAULT_STR_VALUE["Autocmd5"] = "";
		DEFAULT_INT_VALUE["AutocmdEncrypted2"] = 0;
		DEFAULT_INT_VALUE["AutocmdEncrypted3"] = 0;
		DEFAULT_INT_VALUE["AutocmdEncrypted4"] = 0;
		DEFAULT_INT_VALUE["AutocmdEncrypted5"] = 0;
		DEFAULT_INT_VALUE["AutocmdHide2"] = 0;
		DEFAULT_INT_VALUE["AutocmdHide3"] = 0;
		DEFAULT_INT_VALUE["AutocmdHide4"] = 0;
		DEFAULT_INT_VALUE["AutocmdHide5"] = 0;
		DEFAULT_STR_VALUE["AutocmdExpect2"] = "";
		DEFAULT_STR_VALUE["AutocmdExpect3"] = "";
		DEFAULT_STR_VALUE["AutocmdExpect4"] = "";
		DEFAULT_STR_VALUE["AutocmdExpect5"] = "";
		DEFAULT_STR_VALUE["BellWaveFile"] = "";
		DEFAULT_STR_VALUE["BoldFont"] = "";
		DEFAULT_STR_VALUE["BoldFontName"] = "";
		DEFAULT_INT_VALUE["BoldFontCharSet"] = 0;
		DEFAULT_INT_VALUE["BoldFontHeight"] = 0;
		DEFAULT_INT_VALUE["BoldFontIsBold"] = 0;
		DEFAULT_STR_VALUE["Font"] = "Courier New";
		DEFAULT_STR_VALUE["FontName"] = "Courier New";
		DEFAULT_INT_VALUE["FontCharSet"] = 0;
		DEFAULT_INT_VALUE["FontHeight"] = 14;
		DEFAULT_INT_VALUE["FontIsBold"] = 0;
		DEFAULT_STR_VALUE["GSSCustom"] = "";
		DEFAULT_STR_VALUE["LogFileName"] = "putty.log";
		DEFAULT_STR_VALUE["PublicKeyFile"] = "";
		DEFAULT_STR_VALUE["X11AuthFile"] = "";
		DEFAULT_STR_VALUE["WideBoldFont"] = "";
		DEFAULT_STR_VALUE["WideBoldFontName"] = "";
		DEFAULT_INT_VALUE["WideBoldFontCharSet"] = 0;
		DEFAULT_INT_VALUE["WideBoldFontHeight"] = 0;
		DEFAULT_INT_VALUE["WideBoldFontIsBold"] = 0;
		DEFAULT_STR_VALUE["WideFont"] = "";
		DEFAULT_STR_VALUE["WideFontName"] = "";
		DEFAULT_INT_VALUE["WideFontCharSet"] = 0;
		DEFAULT_INT_VALUE["WideFontHeight"] = 0;
		DEFAULT_INT_VALUE["WideFontIsBold"] = 0;
	}
	isInited = true;
}

void do_defaults(const char *session, Conf *conf)
{
    load_settings(session, conf);
}

int sessioncmp(const void *av, const void *bv)
{
    const char *a = *(const char *const *) av;
    const char *b = *(const char *const *) bv;

    /*
     * Alphabetical order, except that "Default Settings" is a
     * special case and comes first.
     */
    if (!strcmp(a, DEFAULT_SESSION_NAME))
	return -1;		       /* a comes first */
    if (!strcmp(b, DEFAULT_SESSION_NAME))
	return +1;		       /* b comes first */

	if (!strcmp(a, GLOBAL_SESSION_NAME))
	return -1;		       /* a comes first */
	if (!strcmp(b, GLOBAL_SESSION_NAME))
	return +1;	

	if (!strcmp(a, ANDROID_DIR_FOLDER_NAME))
		return -1;		       /* a comes first */
	if (!strcmp(b, ANDROID_DIR_FOLDER_NAME))
		return +1;		       /* b comes first */
	if (!memcmp(a, ANDROID_DIR_FOLDER_NAME, strlen(ANDROID_DIR_FOLDER_NAME)) && memcmp(b, ANDROID_DIR_FOLDER_NAME, strlen(ANDROID_DIR_FOLDER_NAME)))
		return -1;
	if (memcmp(a, ANDROID_DIR_FOLDER_NAME, strlen(ANDROID_DIR_FOLDER_NAME)) && !memcmp(b, ANDROID_DIR_FOLDER_NAME, strlen(ANDROID_DIR_FOLDER_NAME)))
		return +1;
		
    /*
     * FIXME: perhaps we should ignore the first & in determining
     * sort order.
     */
    return strcmp(a, b);	       /* otherwise, compare normally */
}

void get_sesslist(struct sesslist *list, int allocate)
{
    char otherbuf[2048];
    int buflen, bufsize, i;
    char *p, *ret;
    void *handle;

    if (allocate) {

	buflen = bufsize = 0;
	list->buffer = NULL;
	if ((handle = gStorage->enum_settings_start()) != NULL) {
	    do {
		ret = gStorage->enum_settings_next(handle, otherbuf, sizeof(otherbuf));
		if (ret) {
		    int len = strlen(otherbuf) + 1;
		    if (bufsize < buflen + len) {
			bufsize = buflen + len + 2048;
			list->buffer = sresize(list->buffer, bufsize, char);
		    }
		    strcpy(list->buffer + buflen, otherbuf);
		    buflen += strlen(list->buffer + buflen) + 1;
		}
	    } while (ret);
	    gStorage->enum_settings_finish(handle);
	}
	list->buffer = sresize(list->buffer, buflen + 1, char);
	list->buffer[buflen] = '\0';

	/*
	 * Now set up the list of sessions. Note that "Default
	 * Settings" must always be claimed to exist, even if it
	 * doesn't really.
	 */

	const int DEFAULT_SESSION_NUM = 5;
	p = list->buffer;
	list->nsessions = DEFAULT_SESSION_NUM;	       /* "Default Settings" counts as one */
	while (*p) {
		if (strcmp(p, GLOBAL_SESSION_NAME) && strcmp(p, DEFAULT_SESSION_NAME) && strcmp(p, ANDROID_DIR_FOLDER_NAME)
			&& strcmp(p, START_LOCAL_SSH_SERVER_NAME) && strcmp(p, LOCAL_SSH_SESSION_NAME))
		list->nsessions++;
	    while (*p)
		p++;
	    p++;
	}

	list->sessions = snewn(list->nsessions + 1, char *);
	list->sessions[0] = GLOBAL_SESSION_NAME;
	list->sessions[1] = DEFAULT_SESSION_NAME;
	list->sessions[2] = ANDROID_DIR_FOLDER_NAME;
	list->sessions[3] = START_LOCAL_SSH_SERVER_NAME;
	list->sessions[4] = LOCAL_SSH_SESSION_NAME;
	p = list->buffer;
	i = DEFAULT_SESSION_NUM;
	while (*p) {
		if (strcmp(p, GLOBAL_SESSION_NAME) && strcmp(p, DEFAULT_SESSION_NAME) && strcmp(p, ANDROID_DIR_FOLDER_NAME)
			&& strcmp(p, START_LOCAL_SSH_SERVER_NAME) && strcmp(p, LOCAL_SSH_SESSION_NAME)){
			list->sessions[i++] = p;
		}
	    while (*p)
		p++;
	    p++;
	}

	qsort(list->sessions, i, sizeof(const char *), sessioncmp);
    } else {
	sfree(list->buffer);
	sfree(list->sessions);
	list->buffer = NULL;
	list->sessions = NULL;
    }
}

char *backup_settings(const char *section,const char* path)
{
    void *sesskey;
    char *errmsg;
	FileStore fileStore(path);
	Conf* cfg = conf_new();

	if (!strcmp(section, START_LOCAL_SSH_SERVER_NAME)) return NULL;
	if (!strcmp(section, LOCAL_SSH_SESSION_NAME)) return NULL;

    sesskey = fileStore.open_settings_w(section, &errmsg);
    if (!sesskey)
	return errmsg;
	load_settings(section, cfg);
    save_open_settings(&fileStore, sesskey, cfg);
    fileStore.close_settings_w(sesskey);
	conf_free(cfg);
    return NULL;
}

char* load_global_ssetting(char* setting, const char* def)
{
	void *sesskey = gStorage->open_global_settings();
	char *ret = gStorage->read_setting_s(sesskey, setting);
	gStorage->close_settings_r(sesskey);
	return ret == NULL ? dupstr(def) : ret ;
}

void save_global_ssetting(char* setting, const char* value)
{
	void *sesskey = gStorage->open_global_settings();
	if (!sesskey)
		return ;
	gStorage->write_setting_s(sesskey, setting, value);
	gStorage->close_settings_w(sesskey);
}

int load_global_isetting(char* setting, int def)
{
	void *sesskey = gStorage->open_global_settings();
	int ret = gStorage->read_setting_i(sesskey, setting, def);
	gStorage->close_settings_r(sesskey);
	return ret;
}

void save_global_isetting(char* setting, int value)
{
	void *sesskey = gStorage->open_global_settings();
	if (!sesskey)
		return;
	gStorage->write_setting_i(sesskey, setting, value);
	gStorage->close_settings_w(sesskey);
}

char* load_ssetting(const char *section, char* setting, const char* def)
{
	TmplStore tmpl_store(gStorage);
	return tmpl_store.load_ssetting(section, setting, def);
}

char* save_ssetting(const char *section, char* setting, const char* value)
{
	void *sesskey;
	char *errmsg;

	if (!setting || !*setting)
		return NULL;
	sesskey = gStorage->open_settings_w(section, &errmsg);
	if (!sesskey)
		return errmsg;
	gStorage->write_setting_s(sesskey, setting, value);
	gStorage->close_settings_w(sesskey);
	return NULL;
}

int load_isetting(const char *section, char* setting, int defvalue)
{
    void *sesskey;
    int res = 0;

    sesskey = gStorage->open_settings_r(section);
	res = gppi_raw(gStorage, sesskey, setting, defvalue);
    gStorage->close_settings_r(sesskey);
    return res;
}

char *save_isetting(const char *section, char* setting, int value)
{
    void *sesskey;
    char *errmsg;

    if (!setting || !*setting) 
        return NULL;
    sesskey = gStorage->open_settings_w(section, &errmsg);
    if (!sesskey)
	return errmsg;
    gStorage->write_setting_i(sesskey, setting, value);
    gStorage->close_settings_w(sesskey);
    return NULL;
}

void move_settings(const char* fromsession, const char* tosession)
{
	Conf* cfg = conf_new();
	load_settings(fromsession, cfg);
	gStorage->del_settings(fromsession);
	char *errmsg = save_settings(tosession, cfg);
	conf_free(cfg);
	if (errmsg){
		return;
	}
}

void copy_settings(const char* fromsession, const char* tosession)
{
    Conf* cfg = conf_new();
	
	load_settings(fromsession, cfg);
	char *errmsg = save_settings(tosession, cfg);
	conf_free(cfg);
	if (errmsg){
		return;
	}
}

int lower_bound_in_sesslist(struct sesslist *list, const char* session)
{
	int first = 0;
	int last = list->nsessions;
	int count = last;
	int it, step;

	while (count > 0)
	{
		it = first; step = count/2; it += step;
		if (sessioncmp(&list->sessions[it], &session) == -1) 
		{ first=++it; count-=step+1;  }
		else count=step;
	}
	return first;
}

int for_grouped_session_do(const char* group_session_name, SessionHandler handler, int max_num)
{
	int success_session_num = 0;
	int session_len = strlen(group_session_name);
	if (*group_session_name && group_session_name[session_len - 1] == '#')
	{
		struct sesslist sesslist;
		get_sesslist(&sesslist, TRUE);
		int first = lower_bound_in_sesslist(&sesslist, group_session_name);
		for (first = first; first < sesslist.nsessions; first++) 
		{
			char* sub_session = sesslist.sessions[first];
			if (strcmp(sub_session, group_session_name) == 0){ continue; }
			if (strncmp(sub_session, group_session_name, session_len)){ break; }
			if (handler(sub_session)){ success_session_num++; }
			if (max_num <= success_session_num){ break; }
		}
		get_sesslist(&sesslist, FALSE);
	}
	else
	{
		if (handler(group_session_name)){ success_session_num++; }
	}
	return success_session_num;
}

void translate_all_session_data()
{
	struct sesslist sesslist;
	get_sesslist(&sesslist, TRUE);
	for (int first = 0; first < sesslist.nsessions; first++)
	{
		char* sub_session = sesslist.sessions[first];
		int data_version = load_isetting(sub_session, "DataVersion", 1);
		if (data_version != DATA_VERSION)
		{
			Conf* cfg = conf_new();
			load_settings(sub_session, cfg);
			char *errmsg = save_settings(sub_session, cfg);
			conf_free(cfg);
		}
	}
	get_sesslist(&sesslist, FALSE);
}

int del_settings(const char *sessionname)
{
	gStorage->del_settings(sessionname);
	return 1;
}
