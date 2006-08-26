#include "grame02_lifo.h"

#include <stdio.h>
#include <stdlib.h>

/*
fixme: could rename lifo
*/


static int CAS32( volatile void *p32, volatile void *oldValue, volatile void *newValue )
{
    unsigned char resultFlags;

    asm{
        mov eax, oldValue
        mov ebx, newValue

        mov esi, p32
        
        lock cmpxchg [esi], ebx

        lahf

        mov resultFlags, ah
    }

    return resultFlags & 0x40;
}


static int CAS64( volatile void *p64, volatile void *oldPtr, volatile unsigned long oldCount,
        void *newPtr, unsigned long newCount )
{
    unsigned char resultFlags;

    asm{
        mov eax, oldPtr
        mov edx, oldCount

        mov ebx, newPtr
        mov ecx, newCount

        mov esi, p64
        
        lock cmpxchg8b [esi]

        lahf

        mov resultFlags, ah
    }

    return resultFlags & 0x40;
}


LIBLFDS_CALLING_CONVENTION(void) grame02_lifo_initialize( grame02_lifo_t* stack )
{
    stack->top = NULL;
}


LIBLFDS_CALLING_CONVENTION(void) grame02_lifo_push( grame02_lifo_t* stack, grame02_lifo_node_t *node )
{
/* B1: loop                                                                   */
    for(;;){
/* B2:  cl->next = lf->top # set the cell next pointer to top of the lifo     */

        /* On IA32 the CAS32 below ensures the visibility of stack->top to all
         * other threads. Otherwise we may need a read barrier here to ensure
         * that the current value of stack->top is being fetched.
         * ==> actually not 100% this is true if CAS32 fails, should check
         */

        node->next = stack->top;
        
/* B3:  if CAS (&lf->top, cl->next, cl) # try to set the top of
                                        #                    the lifo to cell */
/* B4:   break                                                                */
/* B5:  endif                                                                 */

        /* On IA32 CAS has full fence semantics, otherwise we may need a write
         * barrier here to ensure that node->next and node->value are stored
         * before the CAS is executed.
         */
         
        if( CAS32( &stack->top, node->next, node ) )
            break;
/* B6: endloop                                                                */
    }
}


LIBLFDS_CALLING_CONVENTION(grame02_lifo_node_t*) grame02_lifo_pop( grame02_lifo_t* stack )
{
    grame02_lifo_node_t *top, *next;
    unsigned long pop_count;
/* SC1: loop                                                                  */
    for(;;){

        /* On some architectures a read barrier may be necessary here to ensure
         * that fresh values are being read for stack->top and stack->pop_count
         * and also for top->value (because we return top below and value needs
         * to be present for the client to use it).
         */

/* SC2:     head = lf->top # get the top cell of the lifo                     */
/* SC2:     oc = lf->ocount # get the pop operations count                    */
        top = (grame02_lifo_node_t*)stack->top;
        pop_count = stack->pop_count;

/* SC3:     if head == NULL                                                   */
/* SC4:         return NULL # LIFO is empty                                   */
/* SC5:     endif                                                             */
        if( !top )
            return NULL;

        /* If a read barrier is needed at all, one above should ensure that
         * top->next is also up to date when we read it.
         */

/* SC6:     next = head->next # get the next cell of cell                     */
        next = (grame02_lifo_node_t*)top->next;

/* SC7:     if CAS2 (&lf->top, head, oc, next, oc + 1) # try to change both
                                                       # the top of the lifo
                                                       # and pop count        */
/* SC8:         break                                                         */
/* SC9:     endif
                                                      */
        if( CAS64( stack, top, pop_count, next, pop_count + 1 ) )
            break;

/* SC10: endloop                                                              */
    }
/* SC11: return head                                                          */
    return top;
}


LIBLFDS_CALLING_CONVENTION(void) grame02_lifo_test(void)
{
    const char *s = "\n!dlrow olleh";
    grame02_lifo_t stack;
    const char *p;
    grame02_lifo_node_t *node;
   
    grame02_lifo_initialize( &stack );
    
    p = s;
    while( *p ){
        char *p2 = (char*)malloc( 1 );
        *p2 = *p++;
        node = (grame02_lifo_node_t*)malloc( sizeof(grame02_lifo_node_t) );
        node->value = p2;
        grame02_lifo_push( &stack, node );
    }

    node = grame02_lifo_pop( &stack );
    while( node ){
        //free( node ); // actually it isn't safe to free node here, this is just a test so we leak it
        printf( "%c", * ((char*)(node->value)) );
        free( node->value );
        node = grame02_lifo_pop( &stack );
    }
}

