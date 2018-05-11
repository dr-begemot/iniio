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
    ini->sections = NULL;
    ini->comment = NULL;

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
        fprintf(f, "[%s]%s", section->name, section->comment ? section->comment : "\n");
        key = section->keys;
        while (key) {
            fprintf(f, "%s=%s%s", key->name, key->value ? key->value : "", key->comment ? key->comment : "\n");
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
        printf("[%s]%s", section->name, section->comment ? section->comment : "\n");
        key = section->keys;
        while (key) {
            printf("%s=%s%s", key->name, key->value ? key->value : "", key->comment ? key->comment : "\n");
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

static const char*
discard_line (const char *p) {
    while (*p && *p != '\n') {
        p++;
    }
    if (*p && p[1] && p[1] == '\r')
        p++;
    return p;
}

static const char *
trim (const char *p) {
    while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) {
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
        //int d = tolower(*a) - tolower(*b);
        int d = *a - *b;
        if (d != 0 || !*a) {
            return d;
        }
        a++, b++;
    }
}

void
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

static ini_section_t *
create_section (ini_t * ini, const char *section_name) {
    ini_section_t *section = malloc (sizeof (ini_section_t));
    section->name = malloc (strlen(section_name) + 1);
    memcpy (section->name, section_name, strlen(section_name) + 1);
    section->keys = NULL;
    section->next = NULL;
    section->comment = NULL;
    add_section (ini, section);
    return section;
}

static ini_section_t *
find_section (ini_t * ini, const char *section_name) {
    ini_section_t *section = ini->sections;
    while (section) {
        if (!strcmpci (section_name, section->name)) {
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

void
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
    key->name = malloc (strlen(key_name) + 1);
    memcpy (key->name, key_name, strlen(key_name) + 1);
    key->value = NULL;
    key->next = NULL;
    key->comment = NULL;
    add_key (section, key);
    return key;
}

static ini_key_t *
find_key (ini_section_t *section, const char *key_name) {
    ini_key_t *key = section->keys;
    while (key) {
        if (!strcmpci (key_name, key->name)) {
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

static void
set_key (ini_key_t *key, const char* value) {
    if (value) {
        key->value = realloc (key->value, strlen (value) + 1);
        memcpy (key->value, value, strlen (value) + 1);
    }
}

static void
add_str (char **str, const char *start, const char *end) {
    if (str) {
        size_t len = 0;
        if (*str)
            len = strlen (*str);

        *str = realloc (*str, len + (end - start) + 1);
        memcpy (&((*str)[len]), start, end - start);
        (*str)[len + (end - start)] = '\0';
    }
}

static void
add_comment (ini_t *ini, ini_section_t *last_section, ini_key_t *last_key, const char **start, const char *end) {
    if (!last_section && !last_key) {
        add_str (&ini->comment, *start, end);
    } else if (last_key)
        add_str (&last_key->comment, *start, end);
    else
        add_str (&last_section->comment, *start, end);
    *start = NULL; // reset
}

static void
parse_data (ini_t * ini, const char *data) {
    const char *p = data;

    ini_section_t *current = NULL;
    ini_key_t *last_key = NULL;
    ssize_t len;

    const char *comment = NULL;

    char* str = NULL;
    while (*p) {
        // new line
        switch (*p) {
        case '\r':
        case '\n':
            if (comment)
                add_comment (ini, current, last_key, &comment, p);
            p++;
            p = trim (p);
            break;
        case ';':
        case '#':
            // comment lines & comment
            if (!comment)
                comment = p;
            len = my_strcspn (p, "\n", "");
            p += len;
            break;
        case '\t':
        case ' ':
            // comment lines & comment
            if (!comment)
                comment = p;
            p++;
            break;
        case '[':
            // add comment lines & comment to ini
            if (comment)
                add_comment (ini, current, last_key, &comment, p);

            p++;
            len = my_strcspn (p, "]", "#;\n");
            if (len >= 0) {
                str = realloc (str, len + 1);
                str[len] = '\0';
                memcpy (str, p, len);
                current = find_or_create_section (ini, str);
                last_key = NULL;

                p += len + 1;
                len = my_strcspn (p, "#;\n", "");
                p += len;
            } else
                p = discard_line (p); //p += len * -1; // discard line :(
            break;
        default:
            len = my_strcspn (p, "=#; \t\r\n", "");
            if (len >= 0) {
                if (comment)
                    add_comment (ini, current, last_key, &comment, p);

                if (!current)
                    current = create_section (ini, "");

                str = realloc (str, len + 1);
                str[len] = '\0';
                memcpy (str, p, len);
                last_key = find_or_create_key (current, str);
                p += len;
                printf("key = %s\n, p = %s", str, p);

                len = my_strcspn (p, "=#;", "\r\n");
                printf("len = %d\n", len);
                if (len > 0) {
                    p = trim (p + len + 1);
                    len = my_strcspn (p, "\r\n", "#;");
                    printf("len = %d\n", len);
                    if (len > 0) {
                        str = realloc (str, len + 1);
                        str[len] = '\0';
                        memcpy (str, p, len);
                        set_key (last_key, str);
                        p += len;
                    }
                } else
                    p += len * -1; // discard incorrect line
            } else if (len == 0)
                p = discard_line (p);
            else
                p += len * -1; // discard line
        } // switch
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
ini_find (ini_t *ini, const char *section_name, const char *key_name) {
    ini_section_t *section = find_section (ini, section_name);
    if (section) {
        ini_key_t *key = find_key (section, key_name);
        return key;
    }
    return NULL;
}

const char*
ini_gets (ini_t *ini, const char *section_name, const char *key_name, const char *def_val) {
    ini_key_t *key = ini_find (ini, section_name, key_name);
    if (key && key->value)
        return key->value;
    return def_val;
}

int
ini_get_int (ini_t *ini, const char *section_name, const char *key_name, int def_val) {
    return 0;
}

int
ini_get_bool (ini_t *ini, const char *section_name, const char *key_name, int def_val) {
    return 0;
}

void
ini_sets (ini_t *ini, const char *section_name, const char *key_name, const char *val) {
    ini_key_t *key = ini_find (ini, section_name, key_name);
    if (key) {
        key->value = realloc (key->value, strlen (val) + 1);
        memcpy (key->value, val, strlen (val) + 1);
    }
}

void
ini_set_int (ini_t *ini, const char *section_name, const char *key_name, int val) {

}

void
ini_set_bool (ini_t *ini, const char *section_name, const char *key_name, int val) {

}
