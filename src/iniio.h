/**
 * Copyright (c) 2018 dr_begemot
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `iniio.c` for details.
 */

#ifndef INIIO_H
#define INIIO_H

#define INIIO_VERSION "0.1.1"

typedef struct ini_t ini_t;

ini_t*  ini_load (const char *filename);
void    ini_free (ini_t *ini);
int ini_save (ini_t *ini, const char *filename);

void ini_print (ini_t *ini);

const char* ini_get_string (ini_t *ini, const char *section_name, const char *key_name, const char *def_val);
int ini_get_int (ini_t *ini, const char *section_name, const char *key_name, int def_val);
int ini_get_bool (ini_t *ini, const char *section_name, const char *key_name, int def_val);

void ini_set_string (ini_t *ini, const char *section_name, const char *key_name, const char *val);
void ini_set_int (ini_t *ini, const char *section_name, const char *key_name, int val);
void ini_set_bool (ini_t *ini, const char *section_name, const char *key_name, int val);

#endif
