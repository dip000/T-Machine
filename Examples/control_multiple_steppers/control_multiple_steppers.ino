/*   CONTROL MULTIPLE STEPPERS
 *   Tested for FH61925 Stepper motor with the transistor array driver (AccelStepper::FULL4WIRE)
 *   Tested for NEMA17 with the micropulses driver (AccelStepper::DRIVER) configured with minimum connections
 */

#include <AccelStepper.h>
#include "t_machine.h"

// Conversion factors
#define RPM_TO_STEPS_PER_SEC(stepsPerRev) ( stepsPerRev/60.0 )
#define MILLIMETERS_TO_STEPS(stepsPerRev, diameter) ( stepsPerRev/(PI*diameter) )
#define DEGREES_TO_STEPS(stepsPerRev) ( stepsPerRev/360.0 )

// Stepper-specific conversion factors
struct StepperData {
  AccelStepper motor;
  float rpmToStepsPerSecFactor;
  float millimeterToStepsFactor;
  float degreesToStepsFactor;
  float maxRPM;
};

// Assign stepper instances, pins and properties
// 'degreesToStepsFactor' of type DRIVER was inverted so it rotates at the same direction as the FULL4WIRE
const uint8_t totalSteppers = 2;
StepperData steppers[totalSteppers] = {
  {AccelStepper(AccelStepper::FULL4WIRE, 22, 24, 23, 25), RPM_TO_STEPS_PER_SEC(2038),  MILLIMETERS_TO_STEPS(2038,10), DEGREES_TO_STEPS(2038), 15 },
  {AccelStepper(AccelStepper::DRIVER,    26, 27),         RPM_TO_STEPS_PER_SEC(200),   MILLIMETERS_TO_STEPS(200,10),  -DEGREES_TO_STEPS(200), 90 }
};


// Write your program here. Or perhaps an array of programs
// R-strings place line breaks and null terminators automatically. For regular strings place '\n' manually
// This program will extrude 10mm three times on tool0, meanwhile tool1 rotates 360° asynchronusly at 15rpm (A=async)
char program[] = R"=====(
S15 S15
E10 AR360
E10 A
E10 A
)=====";


// T-MACHINE EVENTS --------------------------------------------------------------------------------
void onExtrudeChanged(float millimeters, int tool){
  float steps = millimeters * steppers[tool].millimeterToStepsFactor;
  steppers[tool].motor.move( steps );
  Serial.print("Tool "); Serial.print(tool); Serial.print(" extruded (mm): "); Serial.println(millimeters);
}

void onRotationChanged(float deg, int tool){
  float steps = deg * steppers[tool].degreesToStepsFactor;
  steppers[tool].motor.move( steps );
  Serial.print("Tool "); Serial.print(tool); Serial.print(" rotated (deg): "); Serial.println(deg);
}

void onSpeedChanged(float rpm, int tool){
  float stepsPerSec = rpm * steppers[tool].rpmToStepsPerSecFactor;
  steppers[tool].motor.setSpeed( stepsPerSec );
}

unsigned long onSynchronusWait(){
  // Interpreter needs to know how much time is gonna take to execute the current synchronus processes
  float workTime = 0;
  
  for(int i=0; i<totalSteppers; i++){
    // 'Asynchronus tools' are motors thtat are running in the background and shouldn't be considered in synchronus caclulations
    if( IS_TOOL_ASYNC(i) )
      continue;
    if( steppers[i].motor.speed() <= 0)
      continue;

    // Calculate the remaning time it's gonna take to finish this process and save the max time (since all processes are running in parallel)
    float thisTime = 1000 * abs( steppers[i].motor.distanceToGo() / steppers[i].motor.speed() );
    if( thisTime > workTime )
      workTime = thisTime;
  }
  Serial.print("Work time (ms): "); Serial.println(workTime);
  return workTime;
}


// MAIN -----------------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200); //low bauds jitters the stepper
  t_begin( totalSteppers );

  // Connect relevant events to interpreter
  t_onExtrudeChanged = &onExtrudeChanged;
  t_onRotationChanged = &onRotationChanged;
  t_onSpeedChanged = &onSpeedChanged;
  t_onSynchronusWait = &onSynchronusWait;

  // Initialize steppers
  for(int i=0; i<totalSteppers; i++){
    steppers[i].motor.setMaxSpeed( steppers[i].maxRPM * steppers[i].rpmToStepsPerSecFactor );
    steppers[i].motor.setSpeed( 0 );
  }
  
  // Start program, executes in 't_handle()'
  // To stop it, execute 't_runProgram("")'
  t_runProgram( program );
  Serial.print("Running program:");
  Serial.println( program );
}

void loop() {
  // Run interpreter from either 'program' or by serial. Doesnt block the loop
  t_handle();

  // Applies one step at a time to each stepper. Should never be blocked
  for(int i=0; i<totalSteppers; i++){
    steppers[i].motor.runSpeedToPosition();
  }
}
