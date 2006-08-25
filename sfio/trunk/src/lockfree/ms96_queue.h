#ifndef INCLUDED_MS96_QUEUE_H
#define INCLUDED_MS96_QUEUE_H

/*
    multiple-reader, multiple-writer lock-free FIFO queue


Michael, M. M. and Scott, M. L.,
"Simple, fast and practical non-blocking and blocking concurrent queue algorithms,"
Proceedings of the 15th Annual ACM Symposium on Principles of Distributed
Computing (PODC), pp. 267--275, May 1996.
http://citeseer.ist.psu.edu/michael96simple.html
*/

#define LIBLF_CALLING_CONVENTION( return_type )  return_type __stdcall


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef void* ms96_value_t;     // this is the "payload" data type which is stored in the queue node
                                // the algorithm allows this to be anything, but at present (at least)
                                // we don't allow it to be (perhaps in future we could pass a special
                                // copy function to support other data types.

struct ms96_queue_node_t;
typedef struct ms96_queue_node_t ms96_queue_node_t;

#pragma pack( push, 4 )


typedef struct ms96_tagged_node_pointer_t{
    volatile ms96_queue_node_t *ptr;
    volatile unsigned long count;
} ms96_tagged_node_pointer_t;


typedef struct ms96_queue_node_t{
    ms96_value_t value;
    ms96_tagged_node_pointer_t next;
} ms96_queue_node_t;


typedef struct ms96_queue_t{
    ms96_tagged_node_pointer_t head;
    ms96_tagged_node_pointer_t tail;
}ms96_queue_t;


#pragma pack( pop )


// initialize queue, requires a node
LIBLF_CALLING_CONVENTION(void) ms96_queue_initialize( ms96_queue_t* queue, ms96_queue_node_t* node );

// deinitialize queue, expects queue to be empty and to stay that way
// returns a node
LIBLF_CALLING_CONVENTION(void) ms96_queue_terminate( ms96_queue_t* queue, ms96_queue_node_t** node );


// to engueue, supply a node (allocated from somewhere), and a value
// algorithm constraints: nodes must never be freed to the system, or unmapped
// from memory once they've been passed to enqueue. (not sure if there's also
// a memory class contraint here).
LIBLF_CALLING_CONVENTION(void) ms96_queue_enqueue( ms96_queue_t* queue, ms96_queue_node_t* node, ms96_value_t value );

// dequeue returns a node, and the value
// returns non-zero on success
LIBLF_CALLING_CONVENTION(int) ms96_queue_dequeue( ms96_queue_t* queue, ms96_queue_node_t** node, ms96_value_t* value );

LIBLF_CALLING_CONVENTION(void) ms96_queue_test(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* INCLUDED_MS96_QUEUE_H */
