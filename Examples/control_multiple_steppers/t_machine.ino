/* INTERPRETER FOR T-MACHINE LANGUAGE
 *  - Connect relevant events on setup like: 't_onExtrudeChanged = &onExtrudeChanged'
 *  - Place 't_handle()' inside 'loop()'. Make sure nothing is blocking the handler calls
 *  - Asynchronus tools should be ignored manually on 't_onSynchronusWait()' using 'IS_TOOL_ASYNC()'
 *  - Enable line breaks on serial monitor if you´re gonna use it
 *  - All instructions are synchronus by default unless you specify 'A' as prefix
 *  
 * Example of a program in T-Machine code (T stands for TABLE):
   
   T0 | T1       This is not needed, neither '|' and '+' and '-'. Those are for visualization purposes
  ----+------    
   S2 | S15      Assign tool speeds, they will be remembered until otherwise changed. Program will continue immediately
   E5 | AR90     First tool extrudes 5mm, second rotates asynchonusly 90°. Program will wait for first tool only
   E5 | A        First tool extrudes 5mm, second tool will keep asynchronusly doing its previous job. Program will wait for first tool only
   *  | R90      First tool is idle since it already finished its job, second tool will rotate 90°. Program will finish when second tool finishes
*/

#include "t_machine.h"

// Tool variables
struct Stash {
  uint8_t count = 0;
  uint8_t loops = 0;
  uint8_t index = 0;
  bool stashing = false;
  bool feeding = false;
  //TODO: Pool instructions instead
  char s[ARG_STASH_SIZE][BUFFER_CHAR_SIZE];
};
Stash *_stashes;
float *_speeds;
uint8_t _tools = 0;

// Custom program
unsigned long _progCounter = 0;
String _program = "";

// Streaming
char _argv[BUFFER_CHAR_SIZE];



void t_begin(uint8_t totalTools){
  _speeds = new float[totalTools];
  _stashes = new Stash[totalTools];
  _tools = totalTools;
}

void t_runProgram(String program){
  _program = program;
  _progCounter = 0;
}

void t_handle(){
  // Update time and wait until the interpreter finishes its current job
  static unsigned long previousTime = 0;
  static bool feedQueued = false;

  if( t_waitTime > 0 ){
    t_waitTime -= millis() - previousTime;
    previousTime = millis();
    return;
  }

  
  // Feed before reading from stream sources
  // Feeds are flow control processes (loops, functions, ..) that aren't read continuously from the main program
  if( feedQueued ){
    feedQueued = false;
    for(uint8_t tool=0; tool<_tools; tool++){
      char* argument = _nextFromFeed(tool);
      if( argument != ""){
        feedQueued = true;
        _execute( argument, tool );
      }
    }
    t_waitTime = t_onSynchronusWait();
    previousTime = millis();
    return;
  }
  

  // Stream sources
  bool endOfLine = false;
  bool endOfArgument = false;
  uint8_t tool;
  
  if( Serial.available() )
    _streamBuffer( Serial.read(), endOfArgument, endOfLine, tool );

  else if( _program[_progCounter] != '\0' )
    _streamBuffer( _program[_progCounter++], endOfArgument, endOfLine, tool );


  // Execute streamed processes
  if( endOfArgument ){
    _execute( _argv, tool );
    _clearBuffer( _argv );
  }
  
  if( endOfLine ){
    // If feeds were found, sinchronize times in the next loop
    feedQueued = _isFeedInQueue();
    if( !feedQueued ){
      t_waitTime = t_onSynchronusWait();
      previousTime = millis();
    }
  }
}


void _streamBuffer(char c, bool &endOfArgument, bool &endOfLine, uint8_t &tool){
  static uint8_t argc = 0;
  static uint8_t charc = 0;

  // Ignore carraige returns
  if(c=='\r')
    return;

  // A space or tab is a change in argument, 'argc' must not be higher than what the buffer can hold plus a null terminator character
  if( c==' ' || c== '\t' ){
    if( argc+1 > BUFFER_CHAR_SIZE )
      return;
    endOfArgument = true;
    tool = argc;
    argc++;
    charc=0;
  }
  // A line break is both. It also resets logic
  else if( c=='\n' ){
    endOfLine = true;
    endOfArgument = true;
    tool = argc;
    argc=0;
    charc=0;
  }
  // Buffer otherwise
  else{
    _argv[ charc++ ] = c;
  }  
}

void _execute(char* argument, uint8_t tool){
  if( tool >= _tools )
    return;

  float value;
  char action;
  
  // Defaults to synchronus if not explicited
  if( argument[0] == ASYNCHRONUS ){
    SET_ASYNC( tool );
    action = argument[1];
    value = String(argument).substring(2).toFloat();
  }
  else{
    CLEAR_ASYNC( tool );
    action = argument[0];
    value = String(argument).substring(1).toFloat();
  }

  // Sets speed after action so it can calculate the time accurately and be consistent
  // Not every action can be stashed due unimplemented nested memory allocations
  switch( action ){
    case EXTRUDE:
      if( t_onExtrudeChanged )
        t_onExtrudeChanged( value, tool );
      if( t_onSpeedChanged )
        t_onSpeedChanged( _speeds[tool] , tool );
      if( _stashes[tool].stashing )
        _stash( argument, tool);
      break;
      
    case ROTATE:
      if( t_onRotationChanged )
        t_onRotationChanged( value, tool );
      if( t_onSpeedChanged )
        t_onSpeedChanged( _speeds[tool] , tool );
      if( _stashes[tool].stashing )
        _stash( argument, tool);
      break;
      
    case SPEED:
      _speeds[tool] = value;
      if( t_onSpeedChanged )
        t_onSpeedChanged( value, tool );
      if( _stashes[tool].stashing )
        _stash( argument, tool);
      break;
      
    case TIME:
      if( t_onTimeChanged )
        t_onTimeChanged( value, tool );
      if( t_onSpeedChanged )
        t_onSpeedChanged( _speeds[tool] , tool );
      if( _stashes[tool].stashing )
        _stash( argument, tool);
      break;
      
    case POWER:
      if( t_onPowerChanged )
        t_onPowerChanged( (value>0) , tool ); //true=ON, false=OFF
      if( _stashes[tool].stashing )
        _stash( argument, tool);
      break;
      
    case LAND:
      _stashes[tool].index = 0;
      _stashes[tool].count = 0;
      _stashes[tool].stashing = true;
      break;
    case JUMP:
      _stashes[tool].stashing = false;
      _stashes[tool].feeding = true;
      _stashes[tool].loops = value;
      //_debugStash( tool );
      break;

    case _IDLE:
      //Idle instructions are stashable. Otherwise, feeds wouldn't skip tools after inputing something like "* * * R90"
      if( _stashes[tool].stashing )
        _stash( argument, tool);
  }
}

void _stash(char* argument, uint8_t tool){
  //No need to clear stashes because 1) The buffer it was copied from was already cleared. 2) 'stash.count' limits the total actual usable arguments
  memcpy( _stashes[tool].s[_stashes[tool].count++], argument, BUFFER_CHAR_SIZE );
}

void _debugStash(uint8_t tool){
  Serial.println("______Stashes:");
  for(uint8_t i=0; i<_tools; i++){
    Serial.print("______"); Serial.println(_stashes[tool].s[i]);
  }
}

// Iterator to fetch the next argument inside given stash. If there are no more arguments, returns false
char* _nextFromFeed(uint8_t tool){
  if( !_stashes[tool].feeding )
    return "";
  
  // Restart index if there are more loops
  bool indexIsOutOfReach = (_stashes[tool].index >= _stashes[tool].count);  
  if( indexIsOutOfReach ){
    _stashes[tool].loops--;
    _stashes[tool].index = 0;
    if( _stashes[tool].loops <= 0 ){
      _stashes[tool].feeding = false;
      return "";
    }
  }

  return _stashes[tool].s[ _stashes[tool].index++ ];
}

bool _isFeedInQueue(){
  for(uint8_t tool=0; tool<_tools; tool++)
    if( _stashes[tool].feeding )
      return true;
  return false;
}

void _clearBuffer(char* buff){
  for(uint8_t i=0; i<BUFFER_CHAR_SIZE; i++)
    buff[i] = '\0';
}
