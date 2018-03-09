//Cosmo Marker/Brake Light Control Version 2.0
//Aaron Cake, Copyright © 2015
//http://www.aaroncake.net/
//http://www.aaroncake.net/76cosmo/taillights

//Rights to modify, distribute, re-use are granted as long as credit to original author remains

//Change log
// 1.0 - Initial version
// 1.1 - Moved trigger pin status read to once at beginning of loop, instead of inside each IF statement. 
// 2.0 - Added boolean condition IsSignalLight to determine if code is running signal lights. If true, changes main loop code to ignore halo trigger
//       and switch both arrays in a pre-determined pattern.


//Software runs on ATTINY85 and controls two groups of LEDs by PWM'ing MOSFETs on pins 0 and 1. Input triggers are pin A1 and A2, isloated
// via opto isolators. A1 and A2 are pulled low when active. LEDs form two groups, the "halo" which surrounds the main light area and the 
// "field" which makes up the brake light area inside the halo. Halo is used as a marker light active when headlights are turned on (triggerd
// by 12V headlight signal at optoisolator (A1) and kept at a nominal brightness by PWM. When 12V brake signal applied to optoisolator on
// pin A2, brake lights are triggered at a nominal PWM. Some PWM time is added to halo to increase brightness. Halo is always triggered with
// brake application at increased brightness regardless of whether market lights are on or off.
// If the variable IsSignalLight is set to true, there is a conditional statement in the main loop to ignore the brake light related code and
// run the signal light code. The signal code ignores the halo input and only switches based on the field input. When that input is triggered,
// the code just illuminates the halo, delays a configurable amount, then lights the field. Upon trigger removal, the opposite happens.


// first set up all the variables needed
const int OptoTriggerThreshold = 500; //Opto threshold. Pin is held high by pullup. Trigger drops to about 0V when optoisolator activated

// Pin assignments for ATTINY85. Uncomment to burn to ATTINY85 for production use
// const int HaloOutPin = 0; // LED halo output pin
// const int FieldOutPin = 1; // LED field output pin
// const int HaloTriggerPin = A1; //Opto pin input for halo
// const int FieldTriggerPin = A2; //Opto pin input for field


// Pin assignments for Arduino UNO R3. Uncomment for development
const int HaloOutPin = 3; // LED halo output pin
const int FieldOutPin = 9; // LED field output pin
const int HaloTriggerPin = A0; //Opto pin input for halo
const int FieldTriggerPin = A1; //Opto pin input for field


const int HaloOffBrightness = 0; // PWM brightness values for LEDs turned off, generally zero :-)
const int FieldOffBrightness = 0;
const int HaloOnBrightness = 50; // Activated PWM brightness values for LEDs, max 255
const int FieldOnBrightness = 50;
const int HaloBrakeBrightnessAdder = 50; // Value added to halo PWM brightness when brake activated
const int MaxCelebrationBrightness = 255; // Maximum brightness for LEDs used in opening celebration, max 255
const int LoopFadeDelay = 10; // Delay for each loop iteration in fade to slow down for human eye, mS
const int FadeStep = 5; // Increment step value for LED fades
const int SignalStageDelay = 250; //Delay between illuminating and deluminating the halo and field when in turn signal mode, in mS

//determines if the code is going to control brake lights or signal lights. Set to TRUE if being burned to signal controller. This changes the 
//behaviour of the outputs to ignore the halo input and instead illuminate and deluminate the two LED arrays in a specific pattern.
const boolean IsSignalLight = true;

int HaloTriggerPinStatus = 1000; // Status of the halo trigger pin. Set at 1000 as pin is pulled low on trigger, so this is "off"
int FieldTriggerPinStatus = 1000; // Same as HaloTriggerPin

boolean SignalLightOn = false; // To track whether the signal light is activated

// function to fade up LED on FadeOutputPin. Parameters fairly self explanitory. Max 255 on FadeEndValue, FadeDelayValue in mS
void FadeUpLED (int FadeOutputPin, int FadeStartValue, int FadeEndValue, int FadeStepValue, int FadeDelayValue) {
  for (int OutputPWM = FadeStartValue; OutputPWM <= FadeEndValue; OutputPWM += FadeStepValue) {
    analogWrite(FadeOutputPin, OutputPWM);
    delay(FadeDelayValue);
    // The fade on power up is cool, but all this fancy-pancy stuff needs to stop immediately if the brake or marker light
    // is turned on during to resume normal brake/marker light operation. Check halo and field trigger pins for low signal
    // and bomb out of for loop if detected
    if (analogRead(HaloTriggerPin) < OptoTriggerThreshold || analogRead(FieldTriggerPin) < OptoTriggerThreshold) {
      break;
    }
  }
}

// function to fade down LED on FadeOutputPin. Parameters fairly self explanitory. Max 255 on FadeEndValue, FadeDelayValue in mS
void FadeDownLED (int FadeOutputPin, int FadeStartValue, int FadeEndValue, int FadeStepValue, int FadeDelayValue) {
  for (int OutputPWM = FadeStartValue; OutputPWM >= FadeEndValue; OutputPWM -= FadeStepValue) {
    analogWrite(FadeOutputPin, OutputPWM);
    delay(FadeDelayValue);
    // Stop fade if brake light or marker light is activated, exiting loop
    if (analogRead(HaloTriggerPin) < OptoTriggerThreshold || analogRead(FieldTriggerPin) < OptoTriggerThreshold) {
      break;
    }
  }
}

void setup()
{
  pinMode(HaloOutPin, OUTPUT);
  pinMode(HaloTriggerPin, INPUT);
  pinMode(FieldOutPin, OUTPUT);
  pinMode(FieldTriggerPin, INPUT);

  //Debug: Give two blinks for OK to go
//  digitalWrite(HaloOutPin, HIGH);
//  delay (200);
//  digitalWrite(HaloOutPin, LOW);
//  delay (200);
//  digitalWrite(HaloOutPin, HIGH);
//  delay (200);
//  digitalWrite(HaloOutPin, LOW);
//  digitalWrite(FieldOutPin, HIGH);
//  delay (200);
//  digitalWrite(FieldOutPin, LOW);
//  delay (200);
//  digitalWrite(FieldOutPin, HIGH);
//  delay (200);
//  digitalWrite(FieldOutPin, LOW);
//  delay(2000);

   //Fade the halo on, then the field on for opening celebration. Looks cool.
  FadeUpLED(HaloOutPin,HaloOffBrightness,MaxCelebrationBrightness,FadeStep,LoopFadeDelay);
  FadeUpLED(FieldOutPin,FieldOffBrightness,MaxCelebrationBrightness,FadeStep,LoopFadeDelay);

  //Then fade them down in reverse order
  FadeDownLED(FieldOutPin,MaxCelebrationBrightness,FieldOffBrightness,FadeStep,LoopFadeDelay);
  FadeDownLED(HaloOutPin,MaxCelebrationBrightness,HaloOffBrightness,FadeStep,LoopFadeDelay);

}


void loop()
{

  //Read status of Halo and Field trigger pins and load into status variable. Best to do this once per loop and access
  // status via variable instead of a bunch of analogReads as analogRead is very CPU costly and introduces a noticable
  // flicker into the PWM loop
  HaloTriggerPinStatus = analogRead(HaloTriggerPin);
  FieldTriggerPinStatus = analogRead(FieldTriggerPin);

 //if IsSignalLight is false then go into brake light control mode
 if (IsSignalLight == false) 
 {
   // Control the halo depending on whether marker lights, brake lights, or marker & brake lights on

   // Brake lights and marker lights on. Halo is on and add brightness to it
   if (HaloTriggerPinStatus < OptoTriggerThreshold && FieldTriggerPinStatus < OptoTriggerThreshold)
  {
    analogWrite(HaloOutPin, HaloOnBrightness + HaloBrakeBrightnessAdder);
  }    //Marker lights on, brake lights off. Halo is on at normal brightness
  else if (HaloTriggerPinStatus < OptoTriggerThreshold && FieldTriggerPinStatus > OptoTriggerThreshold)
  {
    analogWrite(HaloOutPin, HaloOnBrightness);
  }  //Marker lights off, brake lights on. Halo is on and add brightness to it.
    else if (HaloTriggerPinStatus > OptoTriggerThreshold && FieldTriggerPinStatus < OptoTriggerThreshold)
  {
    analogWrite(HaloOutPin, HaloOnBrightness + HaloBrakeBrightnessAdder);
  }
  //Marker lights off, brake lights off. Halo is off.
  else if (HaloTriggerPinStatus > OptoTriggerThreshold && FieldTriggerPinStatus > OptoTriggerThreshold)
  {
    analogWrite(HaloOutPin, HaloOffBrightness);
  }


   // The brakes are a lot easier. Just turn them on or off depending on whether the trigger is active
   if (FieldTriggerPinStatus < OptoTriggerThreshold)
  {
    analogWrite(FieldOutPin, FieldOnBrightness);
  }

   if (FieldTriggerPinStatus > OptoTriggerThreshold)
  {
    analogWrite(FieldOutPin, FieldOffBrightness);
  }
}
 
// if IsSignalLight is set to true, then go into signal light mode. Ignore the halo trigger and just control based on field trigger
if (IsSignalLight == true)
{
  //Read the status of the field trigger pin.
  FieldTriggerPinStatus = analogRead(FieldTriggerPin);
  
  // not doing any debouncing here since because flags are used to indicate the status of the lights, and delay is used to form the sequence, any jitter
  // in the input signal is effectively ignored. Also when installed in the car, the lights are going to be controlled by another processor, so if there was
  // bounce in that signal, there are bigger problems!
  
  // if the trigger pin has been pulled low to activate the signal light and the signal lights are turned off (SignalLightOn = F) then go through
  // the turn on sequence
  if (FieldTriggerPinStatus < OptoTriggerThreshold && SignalLightOn == false)
  {
    //turn the halo on
    analogWrite(HaloOutPin, HaloOnBrightness);
    // delay for a little whiile. Yes, delay is evil but do I care the processor can't do anything else for a few hundred mS? Nope.
    delay (SignalStageDelay);
    //turn on the field
    analogWrite(FieldOutPin, FieldOnBrightness);
    // set the flag saying the signal light is on so next time through the loop it doesn't start the sequence again
    SignalLightOn = true;
  }
  
  // if the trigger has been deactivated (pulled high) and the lights are on, do the opposite to turn them off
  if (FieldTriggerPinStatus > OptoTriggerThreshold && SignalLightOn == true)
  {
    //turn the field off
    analogWrite(FieldOutPin, FieldOffBrightness);
    // delay for a little whiile. Yes, delay is evil but do I care the processor can't do anything else for a few hundred mS? Nope.
    delay (SignalStageDelay);
    //turn off the halo
    analogWrite(HaloOutPin, HaloOffBrightness);
    // set the flag saying the signal light is off
    SignalLightOn = false;
  }
  
  
}
 

}

