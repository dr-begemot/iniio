/**
 * Copyright (c) 2018 Grigory Okunev
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

#include "iniio.h"

typedef struct ini_key_t {
    struct ini_key_t *next;
    char* name;
    char* value;
    char* comment;
} ini_key_t;

typedef struct ini_section_t {
    struct ini_section_t *next;
    char* name;
    ini_key_t*  keys;
    char* comment;
} ini_section_t;

//section, const char *key
struct ini_t {
    ini_section_t *sections;
    char* comment;
};

// parse data in place into strings containing section-headers, keys and values using
static void parse_data (ini_t *ini, const char *data);

ini_t*
ini_load (const char *filename) {
    if (!filename)
        return NULL;

    // init ini struct
    ini_t* ini = malloc (sizeof (ini_t));
    memset (ini, 0, sizeof (ini_t));

    FILE *f = fopen (filename, "r");
    if (f) {
        // get file size
        fseek (f, 0, SEEK_END);
        size_t sz = ftell (f);
        rewind (f);

        // load file content into memory, null terminate, init end var
        char *data = malloc (sz + 1);
        data[sz] = '\0';

        size_t n = fread (data, 1, sz, f);
        if (n != sz) {
            goto fail;
        }

        // prepare data
        printf ("parse_data \n");
        parse_data (ini, data);

fail:
        // clean up and return
        free (data);
        fclose (f);
    }

    return ini;
}

void
ini_free (ini_t *ini) {
    void *next;
    ini_section_t *section = ini->sections;
    ini_key_t *key;
    free (ini->comment);
    while (section) {
        key = section->keys;
        free (section->comment);
        while (key) {
            free (key->value);
            free (key->comment);
            next = key->next;
            free (key);
            key = (ini_key_t*) next;
        }
        next = section->next;
        free (section);
        section = (ini_section_t*) next;
    }
    free (ini);
}

int
ini_save (ini_t *ini, const char *filename) {
    ini_section_t *section = ini->sections;
    ini_key_t *key;

    if (!ini)
        return 0;

    FILE *f = fopen (filename, "w+");
    if (!f)
        return -1;
    if (ini->comment)
        fprintf(f, "%s", ini->comment);
    while (section) {
        fprintf(f, "[%s]%s", section->name, /*section->comment ? section->comment :*/ "\n");
        key = section->keys;
        while (key) {
            fprintf(f, "%s=%s%s", key->name, key->value ? key->value : "", /*key->comment ? key->comment :*/ "\n");
            key = key->next;
        }
        section = section->next;
    }

    fclose (f);
    return 0;
}

void
ini_print (ini_t *ini) {
    ini_section_t *section = ini->sections;
    ini_key_t *key;

    if (!ini)
        return;

    if (ini->comment)
        printf("%s", ini->comment);
    while (section) {
        printf("[%s]%s", section->name, /*section->comment ? section->comment :*/ "\n");
        key = section->keys;
        while (key) {
            printf("%s=%s%s", key->name, key->value ? key->value : "", /*key->comment ? key->comment :*/ "\n");
            key = key->next;
        }
        section = section->next;
    }
    /*    printf("ini->comment:\n<%s>\n", ini->comment ? ini->comment : "--none--");
        while (section) {
            printf("section->name:\n<%s>\nsection->comment:\n<%s>\n", section->name, section->comment ? section->comment : "--none--\n");
            key = section->keys;
            while (key) {
                printf("\tkey->name:\n<%s>\n", key->name);
                printf("\tkey->value:\n<%s>\n", key->value ? key->value : "--none--");
                printf("\tkey->comment:\n<%s>\n", key->comment ? key->comment : "--none--\n");
                key = key->next;
            }
            section = section->next;
        }//*/
}

// case insensitive string compare
static int
strcmpci(const char *a, const char *b) {
    for (;;) {
        //int d = tolower(*a) - tolower(*b);
        int d = *a - *b;
        if (d != 0 || !*a) {
            return d;
        }
        a++, b++;
    }
}

static int
strcmpci_span(const char *a, const char *b, const char *b_end) {
    for (;;) {
        //int d = tolower(*a) - tolower(*b);
        int d = *a - *b;
        if (d != 0 || !*a || b == b_end) {
            return d;
        }
        a++, b++;
    }
}

static void
add_section (ini_t * ini, ini_section_t *section) {
    if (!ini->sections) {
        ini->sections = section;
        return;
    }
    ini_section_t *end = ini->sections;
    while (end->next) {
        end = end->next;
    }
    end->next = section;
}

// copy string span with realloc
static char *
span_cpy (char *dist, const char *start, const char *end) {
    if (start > end)
        return dist;
    dist = realloc (dist, (end - start + 1) + 1);
    memcpy (dist, start, end - start + 1);
    dist[end - start + 1] = '\0';
    return dist;
}

static ini_section_t *
create_section (ini_t * ini, const char *section_name) {
    ini_section_t *section = malloc (sizeof (ini_section_t));
    memset (section, 0, sizeof (ini_section_t));
    section->name = malloc (strlen(section_name) + 1);
    memcpy (section->name, section_name, strlen(section_name) + 1);
    add_section (ini, section);
    return section;
}

static ini_section_t *
create_section_span (ini_t * ini, const char *start, const char *end) {
    ini_section_t *section = malloc (sizeof (ini_section_t));
    memset (section, 0, sizeof (ini_section_t));
    section->name = span_cpy (section->name, start, end);
    add_section (ini, section);
    return section;
}

static ini_section_t *
find_section (ini_t * ini, const char *section_name) {
    ini_section_t *section = ini->sections;
    while (section) {
        if (!strcmpci (section->name, section_name)) {
            return section;
        }
        section = section->next;
    }
    return NULL;
}

static ini_section_t *
find_section_span (ini_t * ini, const char *section_name, const char *section_name_end) {
    ini_section_t *section = ini->sections;
    while (section) {
        if (!strcmpci_span (section->name, section_name, section_name_end)) {
            return section;
        }
        section = section->next;
    }
    return NULL;
}

static ini_section_t *
find_or_create_section (ini_t * ini, const char *section_name) {
    ini_section_t *section = find_section (ini, section_name);
    return section ? section : create_section (ini, section_name);
}

static ini_section_t *
find_or_create_section_span (ini_t * ini, const char *section_name, const char *section_name_end) {
    ini_section_t *section = find_section_span (ini, section_name, section_name_end);
    return section ? section : create_section_span (ini, section_name, section_name_end);
}

static void
add_key (ini_section_t *section, ini_key_t *key) {
    if (!section->keys) {
        section->keys = key;
        return;
    }
    ini_key_t *end = section->keys;
    while (end->next) {
        end = end->next;
    }
    end->next = key;
}

static ini_key_t *
create_key (ini_section_t *section, const char *key_name) {
    ini_key_t *key = malloc (sizeof (ini_key_t));
    memset (key, 0, sizeof (ini_key_t));
    key->name = malloc (strlen(key_name) + 1);
    memcpy (key->name, key_name, strlen(key_name) + 1);
    add_key (section, key);
    return key;
}

static ini_key_t *
create_key_span (ini_section_t *section, const char *key_name, const char *key_name_end) {
    ini_key_t *key = malloc (sizeof (ini_key_t));
    memset (key, 0, sizeof (ini_key_t));
    key->name = span_cpy (key->name, key_name, key_name_end);
    add_key (section, key);
    return key;
}

static ini_key_t *
find_key (ini_section_t *section, const char *key_name) {
    ini_key_t *key = section->keys;
    while (key) {
        if (!strcmpci (key->name, key_name)) {
            return key;
        }
        key = key->next;
    }
    return NULL;
}

static ini_key_t *
find_key_span (ini_section_t *section, const char *key_name, const char *key_name_end) {
    ini_key_t *key = section->keys;
    while (key) {
        if (!strcmpci_span (key->name, key_name, key_name_end)) {
            return key;
        }
        key = key->next;
    }
    return NULL;
}

static ini_key_t *
find_or_create_key (ini_section_t *section, const char *key_name) {
    ini_key_t *key = find_key (section, key_name);
    return key ? key : create_key (section, key_name);
}

static ini_key_t *
find_or_create_key_span (ini_section_t *section, const char *key_name, const char *key_name_end) {
    ini_key_t *key = find_key_span (section, key_name, key_name_end);
    if (key) {
        printf ("find_or_create_key_span: %s\n", key->name);
    }
    return key ? key : create_key_span (section, key_name, key_name_end);
}

static void
set_key (ini_key_t *key, const char* value) {
    if (value) {
        key->value = realloc (key->value, strlen (value) + 1);
        memcpy (key->value, value, strlen (value) + 1);
    }
}

static void
set_key_span (ini_key_t *key, const char *start, const char *end) {
    key->value = span_cpy (key->value, start, end);
}

static void
add_str (char **str, const char *apend) {
    if (str) {
        size_t len = 0;
        if (*str)
            len = strlen (*str);
        size_t apend_len = strlen (apend);
        *str = realloc (*str, len + apend_len + 1);
        memcpy (&((*str)[len]), apend, apend_len);
        (*str)[len + apend_len] = '\0';
    }
}

static void
add_str_span (char **str, const char *start, const char *end) {
    if (str && start <= end) {
        size_t len = 0;
        if (*str)
            len = strlen (*str);

        *str = realloc (*str, len + (end - start + 1) + 1);
        memcpy (&((*str)[len]), start, end - start + 1);
        (*str)[len + (end - start + 1)] = '\0';
    }
}

static void
add_comment (ini_t * ini, ini_section_t *last_section, ini_key_t *last_key, const char *comment) {
    if (!last_section && !last_key) {
        add_str (&ini->comment, comment);
    } else if (last_key)
        add_str (&last_key->comment, comment);
    else
        add_str (&last_section->comment, comment);
}

static void
add_comment_span (ini_t * ini, ini_section_t *last_section, ini_key_t *last_key, const char *start, const char *end) {
    if (!last_section && !last_key) {
        add_str_span (&ini->comment, start, end);
    } else if (last_key)
        add_str_span (&last_key->comment, start, end);
    else
        add_str_span (&last_section->comment, start, end);
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

static const char*
discard_line (const char *p) {
    while (*p && *p != '\n') {
        p++;
    }
    if (*p && *p == '\n')
        p++;
    return p;
}

static const char *
trim (const char *p) {
    while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) {
        if (*p == '\n') // new line
            return NULL;
        p++;
    }
    return p;
}

// return next symbol after string or NULL if line ended
static const char *
get_ini_string (const char *p, const char **_begin, const char **_end) {
    p = trim (p);
    if (!p)
        return NULL;

    const char *begin, *end;
    begin = end = p;
    if (*p) {
        if (*p == '\"') {
            do {
                p++;
            } while (*p && *p != '\"');

            if (*p == '\"')
                p++; // next symbol after ""-string
            end = p - 1; // delete stop symbol
        } else {
            while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' &&
                    *p != '#' && *p != ';' && *p != '\"' && *p != ',' &&
                    *p != '[' && *p != ']' && *p != '=')
                p++;
            if (begin != p)
                end = p - 1; // delete stop symbol
            else
                p++;
        }
    }

    if (_begin)
        *_begin = begin;
    if (_end)
        *_end = end;
    printf ("get_ini_string: ");
    for (; begin <= end; begin++) {
        printf ("%c", *begin);
    }
    printf ("\n");

    return p;
}

static void
parse_data (ini_t * ini, const char *data) {
    const char *p = data;

    ini_section_t *current = NULL;
    ini_key_t *last_key = NULL;

    ssize_t len;


    char* str = NULL;

    const char *next, *stop, *begin, *end, *value, *value_end, *comment;
    while (*p) {
        printf ("================\n");
        next = get_ini_string (p, &begin, &end);
        if (!next)
            goto goto_discard_line;
        printf ("symbol pos %zu\n", begin-data);
        switch (*begin) {
        case '[':
            printf ("[:\n");
            next = get_ini_string (next, &begin, &end);
            if (!next)
                goto goto_discard_line;
            stop = 0;
            next = get_ini_string (next, &stop, NULL);
            if (*stop == ']')
                current = find_or_create_section_span (ini, begin, end);
            else
                goto goto_discard_line;
            break;
        case ',':
        case '=':
        case ']':
        case ';':
        case '#':
            goto goto_discard_line;
        default:
            printf ("default: \n");
            next = get_ini_string (next, &stop, NULL);
            if (next && *stop == '=') {
                if (!current)
                    current = create_section (ini, "");
                last_key = find_or_create_key_span (current, begin, end);

              printf ("value pos %zu\n", next-data);
          next = get_ini_string (next, &value, &value_end);
                if (next) {
                    set_key_span (last_key, value, value_end);
                }
                else
                    goto goto_discard_line;
            } else
                goto goto_discard_span;
        }
        printf ("next pos %zu\n", next-data);
        p = next;
        continue;
goto_discard_span:
        printf ("discard_span\n");
        comment = p;
        p = next;
        add_comment_span (ini, current, last_key, comment, p - 1);
        continue;
goto_discard_line:
        printf ("discard_line\n");
        comment = p;
        p = discard_line (p);
        add_comment_span (ini, current, last_key, comment, p - 1);
    } // while
    free (str);
}

static char*
my_itoa (int value, char* result, int base) {
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

ini_key_t *
ini_find (ini_t * ini, const char *section_name, const char *key_name) {
    ini_section_t *section = find_section (ini, section_name);
    if (section) {
        ini_key_t *key = find_key (section, key_name);
        return key;
    }
    return NULL;
}

const char*
ini_gets (ini_t * ini, const char *section_name, const char *key_name, const char *def_val) {
    ini_key_t *key = ini_find (ini, section_name, key_name);
    if (key && key->value)
        return key->value;
    return def_val;
}

int
ini_get_int (ini_t * ini, const char *section_name, const char *key_name, int def_val) {
    return 0;
}

int
ini_get_bool (ini_t * ini, const char *section_name, const char *key_name, int def_val) {
    return 0;
}

void
ini_sets (ini_t * ini, const char *section_name, const char *key_name, const char *val) {
    ini_key_t *key = ini_find (ini, section_name, key_name);
    if (key) {
        key->value = realloc (key->value, strlen (val) + 1);
        memcpy (key->value, val, strlen (val) + 1);
    }
}

void
ini_set_int (ini_t * ini, const char *section_name, const char *key_name, int val) {

}

void
ini_set_bool (ini_t * ini, const char *section_name, const char *key_name, int val) {

}
