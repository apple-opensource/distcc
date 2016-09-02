/* -*- c-file-style: "java"; indent-tabs-mode: nil; fill-column: 78 -*-
 * 
 * distcc -- A simple distributed compiler system
 *
 * Copyright (C) 2003 by Martin Pool <mbp@samba.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include "config.h"

#ifdef HAVE_STDINT_H
#  include <stdint.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "types.h"
#include "distcc.h"
#include "trace.h"
#include "access.h"
#include "exitcode.h"

/**
 * @file
 *
 * Simple IP-based access-control system
 */

/**
 * Interpret a "HOST/BITS" mask specification.  Return @p value and @p mask.
 **/
int dcc_parse_mask(const char *spec,
                   in_addr_t *value,
                   in_addr_t *mask)
{
    int value_len;
    struct in_addr ia;
    int mask_bits;
    char *value_str;
    int matched;
    const char *mask_str;

    value_len = strcspn(spec, "/");

    /* get bit before slash */
    value_str = strdup(spec);
    value_str[value_len] = '\0';
    matched = inet_aton(value_str, &ia);
    
    /* extract and parse value part */
    if (!matched) {
        rs_log_error("can't parse internet address \"%s\"", value_str);
        free(value_str);
        return EXIT_BAD_ARGUMENTS;
    }
    free(value_str);
    *value = ia.s_addr;

    mask_str = &spec[value_len + 1];
    if (spec[value_len] && *mask_str) {
        /* find mask length */
        mask_bits = atoi(mask_str);
        if (mask_bits <= 0 || mask_bits > 32) {
            rs_log_error("invalid mask \"%s\"", mask_str);
            return EXIT_BAD_ARGUMENTS;
        }
        
        /* make mask with the top mask_bits set */
        *mask = htonl(~((1L << (32 - mask_bits)) - 1));
    } else {
        *mask = (in_addr_t) 0xffffffffL;
    }
    return 0;
}


/**
 * Check whether a client ought to be allowed.
 **/
int dcc_check_address(in_addr_t client,
                      in_addr_t value,
                      in_addr_t mask)
{
    if ((client & mask) == (value & mask))
        return 0;
    else
        return EXIT_ACCESS_DENIED;
}
