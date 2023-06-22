#ifndef _T_MACHINE_H
#define _T_MACHINE_H


// Streaming
#define BUFFER_CHAR_SIZE 6
#define ARG_STASH_SIZE 10

// Args
#define EXTRUDE 'E'
#define SPEED 'S'
#define ROTATE 'R'
#define ASYNCHRONUS 'A'
#define POWER 'P'
#define LAND 'L'
#define JUMP 'J'

#define IS_TOOL_ASYNC(tool) ( (t_asyncMask & (1<<tool))==(1<<tool) )
#define SET_ASYNC(tool) ( t_asyncMask |= (1<<tool) )
#define CLEAR_ASYNC(tool) ( t_asyncMask &= ~(1<<tool) )


// public vars
unsigned long t_asyncMask = 0;

// Public events
void (*t_onExtrudeChanged)(float, int);
void (*t_onRotationChanged)(float, int);
void (*t_onSpeedChanged)(float, int);
void (*t_onPowerChanged)(float, int);
unsigned long (*t_onSynchronusWait)();


#endif
