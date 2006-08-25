#include "ms96_queue.h"


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


/*
IA32 Memory Barrier notes:

According to the IA32 reference manual, a locked cmpxchg8b will cause all pending
reads to be executed, and all queued writes to be completed, it is also atomic
with respect to its operands. In other words, this instruction acts like a full
fence. In many algorithms this ensures that any dependent data is updated
(for example, pushing and popping a node from a queue will be guaranteed to
return a node with the correct value already stored in the value field)

One place where the an extra memory barrier is needed is when the following
lock-free idiom is used:

    LOAD X from XSRC
    LOAD Y from YSRC, where YSRC is dependent on XSRC (ie X->YSRC or where XSRC and YSRC were written atomically)
    if( X == XSRC ){ // check that XSRC is still X, therefore Y is consistent with X (ie wasn't derived from the new value of XSRC)
        do something
    }

The above idiom assumes that the load from YSRC has completed when the comparison
between X and XSRC is made. This may not be the case. So a memory barrier is
needed before the comparison, and another after the first read of X (if reading X doesn't require reading X anyway)
This just needs to be a read barrier (LFENCE)
however this is only available on PIV and later, so CPUID is another option.

Need to investigate other less intrusive fence methods that CPUID... (ie lock add [esp], 0 )
*/



// pure ASM implementation?



static int tagged_node_pointer_compare( const ms96_tagged_node_pointer_t* lhs, const ms96_tagged_node_pointer_t* rhs )
{
    return (lhs->ptr == rhs->ptr && lhs->count == rhs->count);
}

static int CAS2( volatile ms96_tagged_node_pointer_t *p, volatile ms96_tagged_node_pointer_t *old, void *newPtr, unsigned long newCount )
{
    unsigned char resultFlags;
    volatile void *oldPtr = old->ptr;
    volatile unsigned long oldCount = old->count;

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


static void testCAS(void)
{
    char *s1 = "s1";
    char *s2 = "s2";

    ms96_tagged_node_pointer_t *a, *b;
    a = (ms96_tagged_node_pointer_t*)malloc( sizeof(ms96_tagged_node_pointer_t) );
    a->ptr = (volatile ms96_queue_node_t*)s1;
    a->count = 1234;

    b = (ms96_tagged_node_pointer_t*)malloc( sizeof(ms96_tagged_node_pointer_t) );
    b->ptr = (volatile ms96_queue_node_t*)s2;
    b->count = 4321;

    if( CAS2( a, a, s1, 1234) )
        printf( "expected success\n" );
    else
        printf( "unexpected failure\n" );
    printf( "expecting s1, 1234: %s, %d\n", (char*)a->ptr, a->count );

    if( CAS2( a, b, s1, 1234) )
        printf( "unexpected success\n" );
    else
        printf( "expected failure\n" );
    printf( "expecting s1, 1234: %s, %d\n", (char*)a->ptr, a->count );

    if( CAS2( a, a, s2, 4321) )
        printf( "expected success\n" );
    else
        printf( "unexpected failure\n" );
    printf( "expecting s2, 4321: %s, %d\n", (char*)a->ptr, a->count );
}

static void read_barrier(void)
{
    // on pentium 4 and above could use LFENCE here
    // the IA32 manual seems to suggest that CPUID should be used as a barrier
    // but also says that a locked ADD will have the same effect (and doesn't
    // clobber memory, so I use that here, which seems to be what linux uses
    asm{
        lock add    [esp], 0  // a noop except that the lock prefix causes memory to be serialised
    }
}



LIBLFDS_CALLING_CONVENTION(void) ms96_queue_initialize( ms96_queue_t *q, ms96_queue_node_t* node )
{
// node = new node()           # Allocate a free node
    // this is a modified version of the algorithm which accepts the node as a parameter

// node->next.ptr = NULL       # Make it the only node in the linked list
// Q->Head = Q->Tail = node    # Both Head and Tail point to it

    node->next.ptr = 0;
    q->head.ptr = q->tail.ptr = node;
}


static void assert_empty(  ms96_queue_t* queue )
{
    ms96_queue_node_t *node;
    ms96_value_t value;

    assert( !ms96_queue_dequeue( queue, &node, &value ) );
}


LIBLFDS_CALLING_CONVENTION(void) ms96_queue_terminate( ms96_queue_t* queue, ms96_queue_node_t** node )
{
    assert_empty( queue );

    *node = (ms96_queue_node_t*)queue->head.ptr;
}


// enqueue(Q: pointer to queue t, value: data type)
LIBLFDS_CALLING_CONVENTION(void) ms96_queue_enqueue( ms96_queue_t* q, ms96_queue_node_t *node, ms96_value_t value )
{
    ms96_tagged_node_pointer_t tail;
    ms96_tagged_node_pointer_t next;
    
// E1: node = new node()           # Allocate a new node from the free list
    // this is a modified version of the algorithm which accepts node as a parameter

// E2: node->value = value         # Copy enqueued value into node
// E3: node->next.ptr = NULL       # Set next pointer of node to NULL

    node->value = value;
    node->next.ptr = 0;
    
// E4: loop                        # Keep trying until Enqueue is done
    for(;;){

// E5:     tail = Q->Tail              # Read Tail.ptr and Tail.count together
// E6:     next = tail.ptr->next       # Read next ptr and count fields together
        tail = q->tail;

        read_barrier(); // ensure that all of tail (both ptr and count) have been read before we read next

        next = tail.ptr->next;

        read_barrier(); // ensure that all of next has been read before we compare tail with q->tail

// E7:     if tail == Q->Tail          # Are tail and next consistent?
        if( tagged_node_pointer_compare( &tail, &q->tail ) ){

// E8:         if next.ptr == NULL         # Was Tail pointing to the last node?
            if( next.ptr == 0 ){

            
// E9:             if CAS(&tail.ptr->next, next, <node, next.count+1>) # Try to link node at the end of the linked list
// E10:                break                      # Enqueue is done. Exit loop
// E11:            endif

                if( CAS2( &tail.ptr->next, &next, node, next.count+1 ) )
                    break;

                
// E12:        else                       # Tail was not pointing to the last node
            }else{
// E13:            CAS(&Q->Tail, tail, <next.ptr, tail.count+1>) # Try to swing Tail to the next node
// E14:

                CAS2( &q->tail, &tail, (void*)next.ptr, tail.count+1 );
            }
// E15:    endif
        }
// E16: endloop
    }

// E17: CAS(&Q->Tail, tail, <node, tail.count+1>) # Enqueue is done. Try to swing Tail to the inserted node

    CAS2( &q->tail, &tail, node, tail.count+1 );
}


// dequeue(Q: pointer to queue t, pvalue: pointer to data type): boolean
LIBLFDS_CALLING_CONVENTION(int) ms96_queue_dequeue( ms96_queue_t* q, ms96_queue_node_t** node, ms96_value_t* value )
{
    ms96_tagged_node_pointer_t head;
    ms96_tagged_node_pointer_t tail;
    ms96_tagged_node_pointer_t next;

// D1: loop # Keep trying until Dequeue is done
    for(;;){
// D2:      head = Q->Head # Read Head
// D3:      tail = Q->Tail # Read Tail
// D4:      next = head->next # Read Head.ptr->next
            head = q->head;

            read_barrier(); // ensure that all of head has been read before we read tail and next
            
            tail = q->tail;
            next = head.ptr->next;

            read_barrier(); // ensure that tail and next have all been read before we compare for consistency

// D5:      if head == Q->Head # Are head, tail, and next consistent?
            if( tagged_node_pointer_compare( &head, &q->head ) ){
            
// D6:          if head.ptr == tail.ptr # Is queue empty or Tail falling behind?
                if( head.ptr == tail.ptr ){

// D7:              if next.ptr == NULL # Is queue empty?
// D8:                  return FALSE # ms96_queue_t is empty, couldn’t dequeue
// D9:              endif
                    if( next.ptr == 0 )
                        return 0;

// D10:             CAS(&Q->Tail, tail, <next.ptr, tail.count+1>) # Tail is falling behind. Try to advance it

                    CAS2( &q->tail, &tail, (void*)next.ptr, tail.count+1 );

// D11:         else # No need to deal with Tail
                }else{

// # Read value before CAS, otherwise another dequeue might free the next node
// D12:             *pvalue = next.ptr->value
                    *value = next.ptr->value;

                    // on IA32 CAS has full-fence semantics, otherwise we would need
                    // a read barrier here to ensure that the read of value above was
                    // finished prior to updating the queue.

// D13:             if CAS(&Q->Head, head, <next.ptr, head.count+1>) # Try to swing Head to the next node
// D14:                 break # Dequeue is done. Exit loop
// D15:             endif

                    if( CAS2( &q->head, &head, (void*)next.ptr, head.count+1 ) )
                        break;

// D16:         endif
                }
// D17:     endif
            }
// D18: endloop
    }
// D19: free(head.ptr) # It is safe now to free the old dummy node
    // this is a modified version of the algorithm which returns node
    *node = (ms96_queue_node_t*)head.ptr;
    
// D20: return TRUE # ms96_queue_t was not empty, dequeue succeeded
    return 1;
}


LIBLFDS_CALLING_CONVENTION(void) ms96_queue_test(void)
{
    const char *s = "hello world!\n";
    ms96_queue_t q;
    const char *p;
    char *p2;
    ms96_queue_node_t *node;

    testCAS();

    ms96_queue_initialize( &q, (ms96_queue_node_t*)malloc( sizeof(ms96_queue_node_t) ) );
    
    p = s;
    while( *p ){
        char *p2 = (char*)malloc( 1 );
        *p2 = *p++;
        ms96_queue_enqueue( &q, (ms96_queue_node_t*)malloc( sizeof(ms96_queue_node_t) ), p2 );
    }

    while( ms96_queue_dequeue( &q, &node, (void**)&p2 ) ){
        //free( node ); // actually it isn't safe to free node here, this is just a test so we leak it
        printf( "%c", * ((char*)(p2)) );
        free( p2 );
    }

    ms96_queue_terminate( &q, &node );
}
