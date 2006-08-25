#ifndef INCLUDED_MESSAGEQUEUE_H
#define INCLUDED_MESSAGEQUEUE_H

#include <new>

#include "ms96_queue.h"


// low level node allocation and deletion, used when creating queues
// nodes allocated by these functions are large enough to be used as nodes
// for both queues and stacks.
void* AllocateNode();
void FreeNode( void *node );


// base class for messages
// messages carry a lock-free node with them (large enough to be used as a node
// on both the queue and stack objects.)
// operator new does some magic so that this node is present when a message
// is constructed.

class Message{
    void *lfNode_;

public:
    static void InitializeAllocator();

    void *GetLFNode() { return lfNode_; }
    void SetLFNode( void *lfNode ){ lfNode_ = lfNode; }

    virtual ~Message() {}

    virtual void Execute() = 0;

    static void* operator new ( size_t size ); // throw( bad_alloc )
    static void operator delete ( void* p ) throw();
};

/*
    idea: could provide a recycle method which turns a Message back into a
    void pointer (calls the dtor under the hood) and also define a placement
    operator new, so we could do something like:

    mq.Post( new (recycle(message)) NextMessage( blah blah blah ) )

    the new operator would check that the raw storage was big enough to
    hold the new message (this could be done by storing the block size at
    the beginning of each memory page (for example)) [or it could free the old
    block and realocate a new one]

    this would save overhead of freeing and immediately reallocating messages
    but perhaps there is not a huge advantage.

*/

class MessageQueue{
    ms96_queue_t queue_;

public:
    MessageQueue();
    ~MessageQueue();

    void Enqueue( Message *message );
    Message *Dequeue();

    /*
        idea: should probably update the above methods to actually execute
        the received messages... or else make an ActiveMessageQueue subclass
        to do it.
        Might be useful also to have a non-lock-free queue class, like
        LocalMessageQueue perhaps even a list template, which still uses
        the same node elements but provides an easier way to traverse the
        structure.

        Also, don't forget the idea of having a separate MessageQueuePort
        object to handle enqueueing objects. This would be used to allow there
        to be two separate message queue subclasses -- one supporting blocking,
        and one only supporting polling.
    */


    static void Test();
};


/*
    define a standard class for queues which loop blocked on a queue, executing
    messages until they are stopped with a termination message.

    define a standard set of functions for implementing increment/decrement
    the same as the win32 api interlocked increment/decrement functions.

    it might be useful to have a freelist class template for handling general
    block allocation tasks (i'm thinking of i/o buffers for example).
*/


#endif
