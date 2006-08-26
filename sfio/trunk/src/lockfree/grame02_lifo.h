#ifndef INCLUDED_GRAME02_LIFO_H
#define INCLUDED_GRAME02_LIFO_H

/*
    multiple-reader, multiple-writer lock-free LIFO stack

    D. Fober, S. Letz, Y. Orlarey
    "Lock-Free Techniques for Concurrent Access to Shared Objects,"
    Actes des Journ�es d'Informatique Musicale JIM2002, Marseille GMEM 2002 Pages 143--150.
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

#pragma pack( push, 4 )


typedef struct grame02_lifo_node_t{
    grame02_lifo_value_t value;
    volatile grame02_lifo_node_t *next;
} grame02_lifo_node_t;


typedef struct grame02_lifo_t{
    volatile grame02_lifo_node_t *top;
    volatile unsigned long pop_count;
}grame02_lifo_t;


#pragma pack( pop )


LIBLFDS_CALLING_CONVENTION(void) grame02_lifo_initialize( grame02_lifo_t* stack );

LIBLFDS_CALLING_CONVENTION(void) grame02_lifo_push( grame02_lifo_t* stack, grame02_lifo_node_t *node );

LIBLFDS_CALLING_CONVENTION(grame02_lifo_node_t*) grame02_lifo_pop( grame02_lifo_t* stack );

LIBLFDS_CALLING_CONVENTION(void) grame02_lifo_test(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INCLUDED_GRAME02_LIFO_H */
