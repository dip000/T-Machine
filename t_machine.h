#ifndef _T_MACHINE_H
#define _T_MACHINE_H


// Streaming
#define BUFFER_CHAR_SIZE 6
#define ARG_STASH_SIZE 10

// Args
#define _IDLE '*'
#define EXTRUDE 'E'
#define SPEED 'S'
#define ROTATE 'R'
#define TIME 'T'
#define ASYNCHRONUS 'A'
#define POWER 'P'
#define LAND 'L'
#define JUMP 'J'

#define IS_TOOL_ASYNC(tool) ( (t_asyncMask & (1<<tool))==(1<<tool) )
#define SET_ASYNC(tool) ( t_asyncMask |= (1<<tool) )
#define CLEAR_ASYNC(tool) ( t_asyncMask &= ~(1<<tool) )


// public vars
unsigned long t_asyncMask = 0;
unsigned long t_waitTime = 0;

// Public events
void (*t_onExtrudeChanged)(float, uint8_t);
void (*t_onRotationChanged)(float, uint8_t);
void (*t_onSpeedChanged)(float, uint8_t);
void (*t_onTimeChanged)(float, uint8_t);
void (*t_onPowerChanged)(bool, uint8_t);
unsigned long (*t_onSynchronusWait)();
void (*t_onProgramFinished)();



#endif
