/*     Simple Stepper Motor Control Exaple Code
 *      
 /*     Simple Stepper Motor Control Exaple Code
 *      
 *  by Dejan Nedelkovski, www.HowToMechatronics.com
 *  
 */
 
// Defines pins numbers
const int stepPin = 3;
const int dirPin = 4; 
int customDelay,customDelayMapped; // Defines variables
 
void setup() {
  // Sets the two pins as Outputs
  Serial.begin(9600);
 
  pinMode(stepPin,OUTPUT);
  pinMode(dirPin,OUTPUT);
 
  digitalWrite(dirPin,HIGH); //Enables the motor to move in a particular direction
}
void loop() {
  
  customDelayMapped = speedUp(); // Gets custom delay values from the custom speedUp function
  // Makes pules with custom delay, depending on the Potentiometer, from which the speed of the motor depends
  digitalWrite(stepPin, HIGH);
  delayMicroseconds(customDelayMapped);
  digitalWrite(stepPin, LOW);
  delayMicroseconds(customDelayMapped);
}
// Function for reading the Potentiometer
int speedUp() {
  int customDelay = analogRead(A0); // Reads the potentiometer
  int newCustom = map(customDelay, 0, 1023, 1, 4000); // Convrests the read values of the potentiometer from 0 to 1023 into desireded delay values (300 to 4000)
  return newCustom;  
}
