#ifndef MessageHandlers_H
#define MessageHandlers_H

#include "DataStructures/Microkernel.h"

void kernelHandler(Message *m);
void vfsHandler(Message *m);
void timerHandler(Message *m);
void loaderHandler(Message *m);

#endif
