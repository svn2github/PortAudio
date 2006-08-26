#include "MessageQueue.h"

#include <cassert>
#include <cstdio>

#include "ms96_queue.h"
#include "grame02_lifo.h"


static size_t nodeSize_;
static grame02_lifo_t nodeFreeList_;
static grame02_lifo_t messageFreeList_;

// for now we limit the message size to 64 bytes, all messages occupy the same space.

// in the future we should probably create some freelists (up to a maximum block size)
// with each freelist being a multiple of the cache line length


#define MAX_MESSAGE_SIZE    64


void Message::InitializeAllocator()
{
    nodeSize_ = std::max( sizeof(ms96_queue_node_t), sizeof(grame02_lifo_node_t) );


    grame02_lifo_initialize( &nodeFreeList_ );
    grame02_lifo_initialize( &messageFreeList_ );
}


void* AllocateNode()
{
    grame02_lifo_node_t *node = grame02_lifo_pop( &nodeFreeList_ );
    if( !node )
        node = reinterpret_cast<grame02_lifo_node_t*>( ::operator new( nodeSize_ ) );

    return node;
}


void FreeNode( void * node )
{
    grame02_lifo_push( &nodeFreeList_, reinterpret_cast<grame02_lifo_node_t*>(node) );
}


// we maintain a freelist of nodes whose values are Messages. when a message
// block is returned, it's node slot points to a node, when a message
// block is released, its node pointer is retrieved and used to place
// the node back on the freelist.

// FIXME: allocations should really be done aligned to cache line boundaries
// and in bigger blocks (ideally)

void* Message::operator new ( size_t size ) // throw( bad_alloc )
{
    assert( size <= MAX_MESSAGE_SIZE ); // if you exceed this value you should retune the system...
    
    grame02_lifo_node_t *node = grame02_lifo_pop( &messageFreeList_ );
    if( node ){
        Message *message = reinterpret_cast<Message*>( node->value ); // pretend we already have a message so we can set the lf node
        message->SetLFNode( node );
        return message;
    }else{
        node = reinterpret_cast<grame02_lifo_node_t*>( AllocateNode() );
        Message *message = reinterpret_cast<Message*>( ::operator new( MAX_MESSAGE_SIZE ) );
        message->SetLFNode( node );
        return message;
    }
}


void Message::operator delete ( void* p ) throw()
{
    Message *message = reinterpret_cast<Message*>( p ); // pretend we already have a message so we can set the lf node
    grame02_lifo_node_t *node = reinterpret_cast<grame02_lifo_node_t*>( message->GetLFNode() );
    assert( node );

    node->value = message;
    grame02_lifo_push( &messageFreeList_, node );
}


MessageQueue::MessageQueue()
{
    ms96_queue_initialize( &queue_, reinterpret_cast<ms96_queue_node_t*>(AllocateNode()) );
}


MessageQueue::~MessageQueue()
{
    ms96_queue_node_t *node;
    ms96_queue_terminate( &queue_, &node );
    FreeNode( node );
}


void MessageQueue::Enqueue( Message *message )
{
    ms96_queue_enqueue( &queue_, reinterpret_cast<ms96_queue_node_t*>(message->GetLFNode()), message );
}


Message *MessageQueue::Dequeue()
{
    ms96_queue_node_t* node;
    ms96_value_t value;
    if( ms96_queue_dequeue( &queue_, &node, &value ) ){
        Message *result = static_cast<Message*>(value);
        result->SetLFNode( node );
        return result;    
    }else{
        return 0;
    }
}


class TestMessage : public Message{
    const char *s_;
public:
    TestMessage( const char *s )
        : s_( s ) {}

    virtual void Execute()
    {
        std::printf( s_ );
        delete this;
    }                  
};


void MessageQueue::Test()
{
    MessageQueue mq;

    mq.Enqueue( new TestMessage( "testing\n" ) );
    mq.Enqueue( new TestMessage( "one\n" ) );
    mq.Enqueue( new TestMessage( "two\n" ) );
    mq.Enqueue( new TestMessage( "three\n" ) );

    while( Message *m = mq.Dequeue() )
        m->Execute();
}
