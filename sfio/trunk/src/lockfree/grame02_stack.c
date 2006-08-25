#include "grame02_stack.h"

#include <stdio.h>
#include <stdlib.h>


static int CAS( volatile void *p, volatile void *oldValue, volatile void *newValue )
{
    unsigned char resultFlags;

    asm{
        mov eax, oldValue
        mov ebx, newValue

        mov esi, p
        
        lock cmpxchg [esi], ebx

        lahf

        mov resultFlags, ah
    }

    return resultFlags & 0x40;
}


static int CAS2( volatile grame02_stack_t *p, volatile void *oldPtr, volatile unsigned long oldCount,
        void *newPtr, unsigned long newCount )
{
    unsigned char resultFlags;

    asm{
        mov eax, oldPtr
        mov edx, oldCount

        mov ebx, newPtr
        mov ecx, newCount

        mov esi, p
        
        lock cmpxchg8b [esi]

        lahf

        mov resultFlags, ah
    }

    return resultFlags & 0x40;
}


LIBLF_CALLING_CONVENTION(void) grame02_stack_initialize( grame02_stack_t* stack )
{
    stack->top = 0;
}


LIBLF_CALLING_CONVENTION(void) grame02_stack_push( grame02_stack_t* stack, grame02_stack_node_t *node )
{
// B1: loop
    for(;;){
// B2:      cl->next = lf->top # set the cell next pointer to top of the lifo
        node->next = stack->top;
// B3:      if CAS (&lf->top, cl->next, cl) # try to set the top of the lifo to cell
// B4:          break
// B5:      endif

        // on IA32 CAS has full fence semantics, otherwise we would need a write barrier
        // here to ensure that node->next and node->value were stored before the CAS is executed

        if( CAS( &stack->top, node->next, node ) )
            break;
// B6: endloop
    }
}


LIBLF_CALLING_CONVENTION(grame02_stack_node_t*) grame02_stack_pop( grame02_stack_t* stack )
{
    grame02_stack_node_t *top, *next;
    unsigned long pop_count;
// SC1: loop
    for(;;){
// SC2:     head = lf->top # get the top cell of the lifo
// SC2:     oc = lf->ocount # get the pop operations count
        top = (grame02_stack_node_t*)stack->top;
        pop_count = stack->pop_count;

// SC3:     if head == NULL
// SC4:         return NULL # LIFO is empty
// SC5:     endif
        if( !top )
            return 0;

// SC6:     next = head->next # get the next cell of cell
        next = (grame02_stack_node_t*)top->next;
        
// SC7:     if CAS2 (&lf->top, head, oc, next, oc + 1) # try to change both the top of the lifo and pop count
// SC8:         break
// SC9:     endif
        if( CAS2( stack, top, pop_count, next, pop_count + 1 ) )
            break;

// SC10: endloop
    }
// SC11: return head
    return top;
}


LIBLF_CALLING_CONVENTION(void) grame02_stack_test(void)
{
    const char *s = "\n!dlrow olleh";
    grame02_stack_t stack;
    const char *p;
    grame02_stack_node_t *node;
   
    grame02_stack_initialize( &stack );
    
    p = s;
    while( *p ){
        char *p2 = (char*)malloc( 1 );
        *p2 = *p++;
        node = (grame02_stack_node_t*)malloc( sizeof(grame02_stack_node_t) );
        node->value = p2;
        grame02_stack_push( &stack, node );
    }

    node = grame02_stack_pop( &stack );
    while( node ){
        //free( node ); // actually it isn't safe to free node here, this is just a test so we leak it
        printf( "%c", * ((char*)(node->value)) );
        free( node->value );
        node = grame02_stack_pop( &stack );
    }
}

