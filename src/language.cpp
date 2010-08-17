/* Multi-language support.
 *
 * (C) 2003-2010 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 */

#include "services.h"
#include "language.h"

/*************************************************************************/

/* The list of lists of messages. */
char **langtexts[NUM_LANGS];

/* The list of names of languages. */
char *langnames[NUM_LANGS];

/* Indexes of available languages: */
int langlist[NUM_LANGS];

/* Order in which languages should be displayed: (alphabetical) */
static int langorder[NUM_LANGS] = {
	LANG_EN_US,		/* English (US) */
	LANG_FR,		/* French */
	LANG_DE,		/* German */
	LANG_IT,		/* Italian */
	LANG_JA_JIS,	/* Japanese (JIS encoding) */
	LANG_JA_EUC,	/* Japanese (EUC encoding) */
	LANG_JA_SJIS,	/* Japanese (SJIS encoding) */
	LANG_PT,		/* Portugese */
	LANG_ES,		/* Spanish */
	LANG_TR,		/* Turkish */
	LANG_CAT,		/* Catalan */
	LANG_GR,		/* Greek */
	LANG_NL,		/* Dutch */
	LANG_RU,		/* Russian */
	LANG_HUN,		/* Hungarian */
	LANG_PL,		/* Polish */
};

/*************************************************************************/

/* Load a language file. */

static int read_int32(int32 *ptr, FILE *f)
{
	int a = fgetc(f);
	int b = fgetc(f);
	int c = fgetc(f);
	int d = fgetc(f);
	if (a == EOF || b == EOF || c == EOF || d == EOF)
		return -1;
	*ptr = a << 24 | b << 16 | c << 8 | d;
	return 0;
}

static void load_lang(int index, const char *filename)
{
	char buf[256];
	FILE *f;
	int32 num, i;

	Alog(LOG_DEBUG) << "Loading language " << index << " from file `languages/" << filename << "'";
	snprintf(buf, sizeof(buf), "languages/%s", filename);
#ifndef _WIN32
	const char *mode = "r";
#else
	const char *mode = "rb";
#endif
	if (!(f = fopen(buf, mode)))
	{
		log_perror("Failed to load language %d (%s)", index, filename);
		return;
	}
	else if (read_int32(&num, f) < 0)
	{
		Alog() << "Failed to read number of strings for language " << index << "(" << filename << ")";
		return;
	}
	else if (num != NUM_STRINGS)
		Alog() << "Warning: Bad number of strings (" << num << " , wanted " << NUM_STRINGS << ") for language " << index << " (" << filename << ")";
	langtexts[index] = static_cast<char **>(scalloc(sizeof(char *), NUM_STRINGS));
	if (num > NUM_STRINGS)
		num = NUM_STRINGS;
	for (i = 0; i < num; ++i)
	{
		int32 pos, len;
		fseek(f, i * 8 + 4, SEEK_SET);
		if (read_int32(&pos, f) < 0 || read_int32(&len, f) < 0)
		{
			Alog() << "Failed to read entry " << i << " in language " << index << " (" << filename << ") TOC";
			while (--i >= 0)
			{
				if (langtexts[index][i])
					free(langtexts[index][i]); // XXX
			}
			free(langtexts[index]);
			langtexts[index] = NULL;
			return;
		}
		if (!len)
			langtexts[index][i] = NULL;
		else if (len >= 65536)
		{
			Alog() << "Entry " << i << " in language " << index << " (" << filename << ") is too long (over 64k) -- corrupt TOC?";
			while (--i >= 0)
			{
				if (langtexts[index][i])
					free(langtexts[index][i]); // XXX
			}
			free(langtexts[index]);
			langtexts[index] = NULL;
			return;
		}
		else if (len < 0)
		{
			Alog() << "Entry " << i << " in language " << index << " (" << filename << ") has negative length -- corrupt TOC?";
			while (--i >= 0)
			{
				if (langtexts[index][i])
					free(langtexts[index][i]); // XXX
			}
			free(langtexts[index]);
			langtexts[index] = NULL;
			return;
		}
		else
		{
			langtexts[index][i] = static_cast<char *>(malloc(len + 1));
			fseek(f, pos, SEEK_SET);
			if (fread(langtexts[index][i], 1, len, f) != len)
			{
				Alog() << "Failed to read string " << i << " in language " << index << "(" << filename << ")";
				while (--i >= 0)
				{
					if (langtexts[index][i])
						free(langtexts[index][i]);
				}
				free(langtexts[index]);
				langtexts[index] = NULL;
				return;
			}
			langtexts[index][i][len] = 0;
		}
	}
	fclose(f);
}

/*************************************************************************/

/* Replace all %M's with "/msg " or "/" */
void lang_sanitize()
{
	int i = 0, j = 0;
	int len = 0;
	char tmp[2000];
	char *newstr = NULL;
	for (i = 0; i < NUM_LANGS; ++i)
	{
		for (j = 0; j < NUM_STRINGS; ++j)
		{
			if (strstr(langtexts[i][j], "%R"))
			{
				len = strlen(langtexts[i][j]);
				strscpy(tmp, langtexts[i][j], sizeof(tmp));
				if (Config->UseStrictPrivMsg)
					strnrepl(tmp, sizeof(tmp), "%R", "/");
				else
					strnrepl(tmp, sizeof(tmp), "%R", "/msg ");
				newstr = strdup(tmp);
				free(langtexts[i][j]); // XXX
				langtexts[i][j] = newstr;
			}
		}
	}
}


/* Initialize list of lists. */

void lang_init()
{
	int i, j, n = 0;

	load_lang(LANG_CAT, "cat");
	load_lang(LANG_DE, "de");
	load_lang(LANG_EN_US, "en_us");
	load_lang(LANG_ES, "es");
	load_lang(LANG_FR, "fr");
	load_lang(LANG_GR, "gr");
	load_lang(LANG_PT, "pt");
	load_lang(LANG_TR, "tr");
	load_lang(LANG_IT, "it");
	load_lang(LANG_NL, "nl");
	load_lang(LANG_RU, "ru");
	load_lang(LANG_HUN, "hun");
	load_lang(LANG_PL, "pl");

	for (i = 0; i < NUM_LANGS; ++i)
	{
		if (langtexts[langorder[i]] != NULL)
		{
			langnames[langorder[i]] = langtexts[langorder[i]][LANG_NAME];
			langlist[n++] = langorder[i];
			for (j = 0; j < NUM_STRINGS; ++j)
			{
				if (!langtexts[langorder[i]][j])
					langtexts[langorder[i]][j] = langtexts[DEF_LANGUAGE][j];
				if (!langtexts[langorder[i]][j])
					langtexts[langorder[i]][j] = langtexts[LANG_EN_US][j];
			}
		}
	}
	while (n < NUM_LANGS)
		langlist[n++] = -1;

	/* Not what I intended to do, but these services are so archa�c
	 * that it's difficult to do more. */
	if ((Config->NSDefLanguage = langlist[Config->NSDefLanguage]) < 0)
		Config->NSDefLanguage = DEF_LANGUAGE;

	if (!langtexts[DEF_LANGUAGE])
		fatal("Unable to load default language");
	for (i = 0; i < NUM_LANGS; ++i)
	{
		if (!langtexts[i])
			langtexts[i] = langtexts[DEF_LANGUAGE];
	}
	lang_sanitize();
}

/*************************************************************************/
/*************************************************************************/

/* Format a string in a strftime()-like way, but heed the user's language
 * setting for month and day names.  The string stored in the buffer will
 * always be null-terminated, even if the actual string was longer than the
 * buffer size.
 * Assumption: No month or day name has a length (including trailing null)
 * greater than BUFSIZE.
 */

int strftime_lang(char *buf, int size, User *u, int format, struct tm *tm)
{
	int language = u && u->Account() ? u->Account()->language : Config->NSDefLanguage;
	char tmpbuf[BUFSIZE], buf2[BUFSIZE];
	char *s;
	int i, ret;

	if (!tm)
		return 0;

	strscpy(tmpbuf, langtexts[language][format], sizeof(tmpbuf));
	if ((s = langtexts[language][STRFTIME_DAYS_SHORT]))
	{
		for (i = 0; i < tm->tm_wday; ++i)
			s += strcspn(s, "\n") + 1;
		i = strcspn(s, "\n");
		strncpy(buf2, s, i);
		buf2[i] = 0;
		strnrepl(tmpbuf, sizeof(tmpbuf), "%a", buf2);
	}
	if ((s = langtexts[language][STRFTIME_DAYS_LONG]))
	{
		for (i = 0; i < tm->tm_wday; ++i)
			s += strcspn(s, "\n") + 1;
		i = strcspn(s, "\n");
		strncpy(buf2, s, i);
		buf2[i] = 0;
		strnrepl(tmpbuf, sizeof(tmpbuf), "%A", buf2);
	}
	if ((s = langtexts[language][STRFTIME_MONTHS_SHORT]))
	{
		for (i = 0; i < tm->tm_mon; ++i)
			s += strcspn(s, "\n") + 1;
		i = strcspn(s, "\n");
		strncpy(buf2, s, i);
		buf2[i] = 0;
		strnrepl(tmpbuf, sizeof(tmpbuf), "%b", buf2);
	}
	if ((s = langtexts[language][STRFTIME_MONTHS_LONG]))
	{
		for (i = 0; i < tm->tm_mon; ++i)
			s += strcspn(s, "\n") + 1;
		i = strcspn(s, "\n");
		strncpy(buf2, s, i);
		buf2[i] = 0;
		strnrepl(tmpbuf, sizeof(tmpbuf), "%B", buf2);
	}
	ret = strftime(buf, size, tmpbuf, tm);
	if (ret == size)
		buf[size - 1] = 0;
	return ret;
}

/*************************************************************************/
/*************************************************************************/

/* Send a syntax-error message to the user. */

void syntax_error(const Anope::string &service, User *u, const Anope::string &command, int msgnum)
{
	const char *str;

	if (!u)
		return;

	str = getstring(u, msgnum);
	notice_lang(service, u, SYNTAX_ERROR, str);
	notice_lang(service, u, MORE_INFO, service.c_str(), command.c_str());
}

const char *getstring(NickAlias *na, int index)
{
	// Default to config
	int langidx = Config->NSDefLanguage;

	// If they are registered (na->nc), and NOT forbidden
	if (na && na->nc && !na->HasFlag(NS_FORBIDDEN))
		langidx = na->nc->language; // set language to their nickcore's language

	return langtexts[langidx][index];
}

const char *getstring(const NickCore *nc, int index)
{
	// Default to config
	int langidx = Config->NSDefLanguage;

	if (nc)
		langidx = nc->language;

	return langtexts[langidx][index];
}

const char *getstring(const User *u, int index)
{
	return getstring(u->Account(), index);
}

const char *getstring(int index)
{
	// Default to config
	int langidx = Config->NSDefLanguage;

	return langtexts[langidx][index];
}

/*************************************************************************/