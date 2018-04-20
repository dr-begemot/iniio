/**
 * Copyright (c) 2018 dr_begemot
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "iniio.h"

typedef struct key_t {
    struct key_t *next;
    char* name;
    char* value;
} key_t;

typedef struct section_t {
    struct section_t *next;
    char*   name;
    key_t*  keys;
} section_t;

//section, const char *key
struct ini_t {
    section_t *sections;
};

// parse data in place into strings containing section-headers, keys and values using
static void parse_data (ini_t *ini, const char *data);

ini_t*
ini_load (const char *filename) {
    if (!filename)
        return NULL;

    FILE *f = fopen (filename, "r");
    if (!f)
        return NULL;

    // init ini struct
    ini_t* ini = malloc (sizeof (ini_t));
    ini->sections = NULL;

    // get file size
    fseek (f, 0, SEEK_END);
    size_t sz = ftell (f);
    rewind (f);

    // load file content into memory, null terminate, init end var
    char *data = malloc (sz + 1);
    data[sz] = '\0';

    n = fread (data, 1, sz, f);
    if (n != sz) {
        goto fail;
    }

    // prepare data
    parse_data (ini, data);
    free (data);

    // clean up and return
    fclose (f);
    return ini;

fail:
    if (f)
        fclose (f);
    if (ini)
        ini_free (ini);
    return NULL;
}

static char*
discard_line (const char *p) {
    while (*p && *p != '\n') {
        p++;
    }
    return p;
}

// returns fail if no one of endchars found
ssize_t
my_strcspn (const char* p, const char* endchars, const char* stopchars) {
    ssize_t cnt = 0;
    size_t len = strlen(endchars);
    size_t stop = strlen(stopchars);
    while (p[cnt]) {
        for (int i = 0; i < stop; i++) {
            if (p[cnt] == stopchars[i])
                return cnt * -1;
        }
        for (int i = 0; i < len; i++) {
            if (p[cnt] == endchars[i])
                return cnt;
        }
        cnt++;
    }
    return cnt * -1;
}

// case insensitive string compare
static int
strcmpci(const char *a, const char *b) {
    for (;;) {
        int d = tolower(*a) - tolower(*b);
        if (d != 0 || !*a) {
            return d;
        }
        a++, b++;
    }
}

static section_t *
create_section (ini_t * ini, const char *section_name) {
    section_t *section = malloc (sizeof (section_t));
    section->name = malloc (strlen(section_name) + 1);
    memcpy (section->name, section_name, strlen(section_name) + 1);
    section->keys = NULL;
    section->next = ini->sections;
    ini->sections = section;
    return section;
}

static section_t *
find_or_create_section (ini_t * ini, const char *section_name) {
    section_t *section = ini->sections;
    while (section) {
        if (!strcmpci (section_name, sections->name)) {
            return section;
        }
        section = sections->next;
    }
    return create_section (ini, section_name);
}

static key_t *
create_key (section_t *section, const char *key_name) {
    key_t *key = malloc (sizeof (key_t));
    key->name = malloc (strlen(key_name) + 1);
    memcpy (key->name, key_name, strlen(key_name) + 1);
    key->value = NULL;
    key->next = section->keys;
    section->keys = key;
    return key;
}

static key_t *
find_or_create_key (section_t *section, const char *key_name) {
    key_t *key = section->keys;
    while (key) {
        if (!strcmpci (key_name, key->name)) {
            return key;
        }
        key = key->next;
    }
    return create_key (ini, section_name);
}

static void
set_key (key_t *key, const char* value) {
    if (value) {
        key->value = realloc (key->value, sizeof (value) + 1);
        memcpy (key->value, value, sizeof (value) + 1);
    }
}

static const char *
trim (const char *str) {
    while (*str == ' ' || *str == '\t')
        str++;
    return str;
}

static void
parse_data (ini_t * ini, const char *data) {
    const char *p = data;
    section_t *current = NULL;
    ssize_t len;
    while (*p) {
        switch (*p) {
        case '\r':
        case '\n':
        case '\t':
        case ' ':
            p++;
            break;
        case '[':
            p++;
            len = my_strcspn (p, "]", "\n");
            if (len >= 0) {
                char* str = malloc (len + 1);
                str[len] = '\0';
                memcpy (str, p, len);
                current = find_or_create_section (ini, str);
                free (str);
                p = discard_line(p + len);
            } else
                p += len * -1;
            break;
        default:
            len = my_strcspn (p, "=; \t\r\n", "");
            if (len > 0) {
                if (!current) {
                    current = create_section (ini, "");
                }
                char* str = malloc (len + 1);
                str[len] = '\0';
                memcpy (str, p, len);
                key_t *key = find_or_create_key (current, str);
                p += len;

                len = my_strcspn (p, "=", "\r\n;");
                if (len >= 0) {
                    p = trim (p + len + 1);
                    len = my_strcspn (p, "; \t\r\n", "");
                    if (len > 0) {
                        str = realloc (len + 1);
                        str[len] = '\0';
                        memcpy (str, p, len);
                        set_key (key, str);
                    }
                }
                free (str);
            }
        case '; ':
            p = discard_line(p);
            break;
        } // switch
    } // while
}