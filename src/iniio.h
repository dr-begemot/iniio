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

#endif
