/*
 * Lock free data structures library
 * Multiple-reader, multiple-writer lock-free LIFO stack
 * Copyright (c) 2006 Ross Bencina <rossb@audiomulch.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

 /*
 * The text above constitutes the entire Lock Free Data Structures Library
 * license; however, the community also makes the following non-binding
 * requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version. It is also 
 * requested that these non-binding requests be included along with the 
 * license above.
 */
 
#ifndef INCLUDED_GRAME02_LIFO_H
#define INCLUDED_GRAME02_LIFO_H

/*
    see:
    
    D. Fober, S. Letz, Y. Orlarey
    "Lock-Free Techniques for Concurrent Access to Shared Objects,"
    Actes des Journées d'Informatique Musicale JIM2002, Marseille GMEM 2002 Pages 143--150.
    ftp://ftp.grame.fr/pub/Documents/fober-JIM2002.pdf

    actually i think this is the IBM freelist algorithm, but I havn't found the reference yet
*/


#define LIBLFDS_CALLING_CONVENTION( return_type )  return_type __stdcall

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


typedef void* grame02_lifo_value_t;     /* this is the "payload" data type which is stored in the stack node */

struct grame02_lifo_node_t;
typedef struct grame02_lifo_node_t grame02_lifo_node_t;

#pragma pack( push, 4 )     /* our assembler versions assume 4 byte alignment */


typedef struct grame02_lifo_node_t{
    volatile grame02_lifo_node_t *next;
    grame02_lifo_value_t value;
} grame02_lifo_node_t;


typedef struct grame02_lifo_t{
    volatile grame02_lifo_node_t *top;
    volatile unsigned long pop_count;
}grame02_lifo_t;


#pragma pack( pop )


LIBLFDS_CALLING_CONVENTION(void) grame02_lifo_initialize( grame02_lifo_t* lifo );

LIBLFDS_CALLING_CONVENTION(void) grame02_lifo_push( grame02_lifo_t* lifo, grame02_lifo_node_t *node );

LIBLFDS_CALLING_CONVENTION(grame02_lifo_node_t*) grame02_lifo_pop( grame02_lifo_t* lifo );

LIBLFDS_CALLING_CONVENTION(void) grame02_lifo_test(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INCLUDED_GRAME02_LIFO_H */

