
#pragma hdrstop
#include <condefs.h>

#include <iostream>

#include "ms96_queue.h"
#include "grame02_stack.h"
#include "MessageQueue.h"

//---------------------------------------------------------------------------
USEUNIT("ms96_queue.c");
USEUNIT("grame02_stack.c");
USEUNIT("MessageQueue.cpp");
//---------------------------------------------------------------------------
#pragma argsused
int main(int argc, char* argv[])
{

    ms96_queue_test();
    grame02_stack_test();

    MessageQueue::Test();

    std::cout << "finished.\n";

    char c;
    std::cin >> c;
    return 0;
}
 