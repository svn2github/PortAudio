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
 
#include "grame02_lifo.h"

#include <stdio.h>
#include <stdlib.h>

/*
    Note that the pseudocode in the comments is quoted from the article
    referenced in the header file; however, the variable names have been
    changed to be consistent with those used in the C implementation.
*/

/* Set one of the following to 1 to enable the desired implementation */

#define GRAME02_LIFO_C_IMPLEMENTATION                   0
#define GRAME02_LIFO_MSVC_IA32_INLINE_IMPLEMENTATION    1
#define GRAME02_LIFO_GCC_IA32_INLINE_IMPLEMENTATION     0


#if GRAME02_LIFO_C_IMPLEMENTATION

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


LIBLFDS_CALLING_CONVENTION(void) grame02_lifo_initialize( grame02_lifo_t* lifo )
{
    lifo->top = NULL;
}


LIBLFDS_CALLING_CONVENTION(void) grame02_lifo_push( grame02_lifo_t* lifo, grame02_lifo_node_t *node )
{
/* B1: loop                                                                   */
    for(;;){
/* B2:  node->next = lifo->top # set the cell next pointer to top of the lifo */

        /* On IA32 the CAS32 below, and the CAS64 in grame02_lifo_pop ensures
         * the visibility of stack->top to all other threads. Otherwise we may
         * need a read barrier here to ensure that the current value of
         * stack->top is being fetched.
         */

        node->next = lifo->top;
        
/* B3:  if CAS (&lifo->top, node->next, node ) # try to set the top of
                                        #                    the lifo to cell */
/* B4:   break                                                                */
/* B5:  endif                                                                 */

        /* On IA32 CAS has full fence semantics, otherwise we may need a write
         * barrier here to ensure that node->next and node->value are stored
         * before the CAS is executed.
         */
         
        if( CAS32( &lifo->top, node->next, node ) )
            break;
/* B6: endloop                                                                */
    }
}


LIBLFDS_CALLING_CONVENTION(grame02_lifo_node_t*) grame02_lifo_pop( grame02_lifo_t* lifo )
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

/* SC2:     top = lifo->top # get the top node of the lifo                    */
/* SC2:     pc = lifo->pop_count # get the pop operations count               */
        top = (grame02_lifo_node_t*)lifo->top;
        pop_count = lifo->pop_count;

/* SC3:     if top == NULL                                                    */
/* SC4:         return NULL # LIFO is empty                                   */
/* SC5:     endif                                                             */
        if( !top )
            return NULL;

        /* If a read barrier is needed at all, one above should ensure that
         * top->next is also up to date when we read it.
         */

/* SC6:     next = top->next # get the next node of the top node              */
        next = (grame02_lifo_node_t*)top->next;

/* SC7:     if CAS2 (&lifo->top, top, pc, next, pc + 1) # try to change both
                                                        # the top of the lifo
                                                        # and pop count       */
/* SC8:         break                                                         */
/* SC9:     endif
                                                      */
        if( CAS64( lifo, top, pop_count, next, pop_count + 1 ) )
            break;

/* SC10: endloop                                                              */
    }
/* SC11: return top                                                           */
    return top;
}

#endif /* GRAME02_LIFO_C_IMPLEMENTATION */


#if GRAME02_LIFO_MSVC_IA32_INLINE_IMPLEMENTATION

/*
    STDCALL passes arguments right-to-left, and return the value in eax.
    esi, edi, ebx and ebp need to be saved on the stack
    which means that eax, ecx and edx can be trashed by the function
    The called function cleans the stack.

    FASTCALL passes the first two arguments in ECX and EDX. The asm versions
    have been written to load the parameters into ECX, EDX so it would be
    really easy to change to fastcall if there was going to be a benefit

    The IA32 compare-exchange functions have some constraints about which
    registers can be used as parameters. We use this to determine our register
    allocation choices.

    CMPXCHG r/m32, r32
        r/m32   -- 32 bit memory to update
        eax     -- old value
        r32     -- new value

        Compare EAX with r/m32. If equal, ZF is set and r32 is loaded into r/m32.
        Else, clear ZF and load r/m32 into EAX

    CMPXCHG8B m64
        m64     -- 64 bit memory to update
        EDX:EAX -- old value
        ECX:EBX -- new value

        Compare EDX:EAX with m64. If equal, set ZF and load ECX:EBX into m64.
        Else, clear ZF and load m64 into EDX:EAX.

    In general we avoid creating a proper stack frame as much as possible
    to try to keep things efficient.
*/


__declspec(naked)
LIBLFDS_CALLING_CONVENTION(void) grame02_lifo_initialize( grame02_lifo_t* lifo )
{
    (void) lifo;
    
    __asm{
        mov     ecx, [esp + 4]
        mov     [ecx], 0        // lifo->top = NULL;
        ret     4
    }
}


__declspec(naked)
LIBLFDS_CALLING_CONVENTION(void) grame02_lifo_push( grame02_lifo_t* lifo, grame02_lifo_node_t *node )
{
    (void) lifo;
    (void) node;
    
    // this function is declared __declspec(naked) so we can handle setting up
    // the stack frame ourselves.
    // we don't clobber any callee save registers so we don't need to save any.
    // we don't make any calls or allocate any locals so we don't need to set
    // up the ebp frame pointer, we can reference our parameters directly off esp

    // [esp]                // return address
    // dword ptr [esp+4]    // lifo
    // dword ptr [esp+8]    // node

    __asm{
        mov     ecx, [esp+4]    // lifo ptr into ecx, this is also the address of lifo->top
        mov     edx, [esp+8]    // node ptr into edx, this is also the address of node->next
        mov     eax, [ecx]      // value of lifo->top into eax
        
/* B1: loop                                                                   */
push_loop:

/* B2:  node->next = lifo->top # set the cell next pointer to top of the lifo */

        mov     [edx], eax  // lifo->top into node->next

/* B3:  if CAS (&lifo->top, node->next, node ) # try to set the top of
                                        #                    the lifo to cell */
/* B4:   break                                                                */
/* B5:  endif                                                                 */

        // ecx contains ptr to lifo (a.k.a. &lifo->top)
        // eax contains the previously read value of lifo->top (a.k.a. node->next)
        // edx contains node
        lock cmpxchg [ecx], edx     // sets ZF on success, copies [esi] to eax on failure
                                    // therefore on failure eax contains new lifo->top
        jnz     push_loop
/* B6: endloop                                                                */

        ret     8
    }
}


__declspec(naked)
LIBLFDS_CALLING_CONVENTION(grame02_lifo_node_t*) grame02_lifo_pop( grame02_lifo_t* lifo )
{
    (void) lifo;
    
    // we use the same registers which are forced by CMPXCHG8B, with lifo in esi
    // eax <= top
    // edx <= pop_count
    // ebx <= next
    // ecx <= pop_count + 1

    // result goes in eax

    __asm{

    // this version implements a fastpath for empty lifos: we check
    // whether lifo->top is NULL prior to saving callee-save registers on the
    // stack. 

    // the following code costs us 1 reg-reg move compared to a simpler version
    // with no fastpath. it can process an empty lifo in 5 instead of 10
    // instructions. in summary, it unrolls the first retry loop and hoists the
    // first empty-lifo test outside the register saving block.
    
        mov     ecx, [esp+4]    // lifo ptr into edx (a caller save register)

/* SC2:     top = lifo->top # get the top node of the lifo                    */
        mov     eax, [ecx]      // lifo->top into eax

/* SC3:     if top == NULL                                                    */
/* SC4:         return NULL # LIFO is empty                                   */
/* SC5:     endif                                                             */
        cmp     eax, 0          // if lifo->top is NULL, return NULL
        jnz     pop_pre_loop
        ret     4

pop_pre_loop:
        push    ebx             // save callee-save registers on stack
        push    esi             // save callee-save registers on stack
        mov     esi, ecx        // lifo ptr into esi
        
/* SC2:     pop_count = lifo->pop_count # get the pop operations count        */
        mov     edx, [esi + 4]  // lifo->pop_count into edx

/* SC6:     next = top->next # get the next node of top                       */

        mov     ebx, [eax]      // put top->next into ebx
        mov     ecx, edx
        inc     ecx             // pop_count + 1 into ecx

/* SC7:     if CAS2 (&lifo->top, top, pc, next, pc + 1)  # try to change both
                                                         # the top of the lifo
                                                         # and pop count      */
                                                         
        lock cmpxchg8b [esi]    // on failure loads (eax, edx) from [esi]
                                // ready for next iteration otherwise eax still
                                // contains top ready to return
        jz     pop_return

/* SC1: loop                                                                  */
pop_loop:

/* SC3:     if top == NULL                                                    */
/* SC4:         return NULL # LIFO is empty                                   */
/* SC5:     endif                                                             */

        cmp     eax, 0          // test for NULL top value
        jz     pop_return       // eax is null so our return value is already set

/* SC6:     next = top->next # get the next node of top                       */

        mov     ebx, [eax]      // put top->next into ebx
        mov     ecx, edx
        inc     ecx             // pop_count + 1 into ecx

/* SC7:     if CAS2 (&lifo->top, top, pc, next, pc + 1)  # try to change both
                                                         # the top of the lifo
                                                         # and pop count      */
                                                         
        lock cmpxchg8b [esi]    // on failure loads (eax, edx) from [esi]
                                // ready for next iteration otherwise eax still
                                // contains top ready to return
        jnz     pop_loop
/* SC8:         break                                                         */
/* SC9:     endif
/* SC10: endloop                                                              */
/* SC11: return top                                                           */

pop_return:
        pop     esi
        pop     ebx
        ret     4               // return old top (still in eax)
    }

    /* ignore "Function should return a value" compiler warning               */
}

#endif /* GRAME02_LIFO_C_IMPLEMENTATION */

#if GRAME02_LIFO_GCC_IA32_INLINE_IMPLEMENTATION

// would be nice if someone who knows IA32 assembler could review the above
// for any obvious optimisations/reorderings before we convert to GCC format

#endif /* GRAME02_LIFO_GCC_IA32_INLINE_IMPLEMENTATION */


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

