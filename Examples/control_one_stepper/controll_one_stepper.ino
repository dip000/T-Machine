/*   ROTATE ONE STEPPER
 *	 Tested for FH61925 Stepper motor with the transistor array driver
 */

#include <AccelStepper.h>
#include "t_machine.h"

// Stepper properties
#define STEPS_PER_REV 2038
#define HEAD_DIAMETER_MILLIMETERS 10
#define MAX_RPM 15

// Precalculate conversion factors
const float millimeterToStepsFactor = STEPS_PER_REV/(PI*HEAD_DIAMETER_MILLIMETERS);
const float degreesToStepsFactor = STEPS_PER_REV/360.0;
const float rpmToStepsPerSecFactor = STEPS_PER_REV/60.0;

// Pins with step sequence
AccelStepper stepper(AccelStepper::FULL4WIRE, 22, 24, 23, 25);

// Write your program here. Or perhaps an array of programs
// R-strings place line breaks and null terminators automatically. For regular strings place '\n' manually
// This program will extrude 10mm, then rotate 90° at 15rpm. Then repeat 2 more times (L=land, J=jump)
const char program[] = R"=====(
S15
L
E10
R90
J2
)=====";


// T-MACHINE EVENTS --------------------------------------------------------------------------------
void onExtrudeChanged(float millimeters, int _tool){
  stepper.move( millimeters*millimeterToStepsFactor );
  Serial.print("Extruding (mm): "); Serial.println(millimeters);
}

void onRotationChanged(float deg, int _tool){
  stepper.move( deg*degreesToStepsFactor );
  Serial.print("Rotating (deg): "); Serial.println(deg);
}

void onSpeedChanged(float rpm, int _tool){
  stepper.setSpeed( rpm*rpmToStepsPerSecFactor );
}

unsigned long onSynchronusWait(){
  // Interpreter needs to know how much time is gonna take to execute the current synchronus process
  if( stepper.speed() == 0 )
    return 0;
  
  float workTime = 1000 * abs( stepper.distanceToGo() / stepper.speed() );
  Serial.print("Work time (ms): "); Serial.println(workTime);
  return workTime;
}


// MAIN -----------------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200); //low bauds jitters the stepper
  t_begin( 1 ); //Total steppers = 1

  // Connect relevant events to interpreter
  t_onExtrudeChanged = &onExtrudeChanged;
  t_onRotationChanged = &onRotationChanged;
  t_onSpeedChanged = &onSpeedChanged;
  t_onSynchronusWait = &onSynchronusWait;

  // Initialize stepper
  stepper.setMaxSpeed( MAX_RPM*rpmToStepsPerSecFactor );
  stepper.setSpeed( 0 );
  
  // Start program, executes in 't_handle()'
  // To stop it, execute 't_runProgram("")'
  t_runProgram( program );
  Serial.print("Running program:");
  Serial.println( program );
}

void loop() {
  // Run interpreter from either 'program' or by serial. Doesnt block the loop
  t_handle();

  // Applies one step at a time. Should never be blocked
  stepper.runSpeedToPosition();
}
