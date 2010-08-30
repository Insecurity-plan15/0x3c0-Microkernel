#ifndef POSIXInterface_H
#define POSIXInterface_H

#include <SystemCalls/SystemCalls.h>

#define POSIXSystemCalls    2

namespace SystemCalls
{
    namespace POSIX
    {
        class Internal
        {
        private:
            Internal();
            ~Internal();
        public:
            SystemCallMethod(fork);
            SystemCallMethod(execve);
        };

        class SystemCallInterrupt : public InterruptSink
        {
        private:
            SystemCall calls[POSIXSystemCalls];
            void receive(StackStates::Interrupt *stack);
        public:
            SystemCallInterrupt();
            ~SystemCallInterrupt();
        };
    }
}

#endif
