#include <Servo.h>  
//REMEBER THE INPUT OF THE LIMIT SWITCH IS: (INPUT_PULLUP)
//I declare the servo's name 
Servo CPencil;

//Servo's up and down
int up = 180;
int down = 40;
//I declare the direction and the pin in which to give the impulse for the X axis
byte dirPinX = 5;
byte stepPinX = 2;

//I declare the direction and the pin in which to give the impulse for the Y axis
byte dirPinY = 6;
byte stepPinY = 3;

int enPin = 8;
//measures in mm
const float beltXLength = 1040.f; 
const float beltYLength = 1040.f; 
const float stepLength = 1; 

//they must be resetted at every turn off
float currentX = 0.f; 
float currentY = 0.f;

//time between two pulses
int pulseWidthMicros = 150;  


void moveTo(float x, float y)
{
  
  float xToMove = x - currentX;
  float yToMove = y - currentY;
  doStep(xToMove, yToMove);
  currentX = x;
  currentY = y;
}

void autoHome()
{
  moveTo(-200 , 0);
  if(digitalRead(9))
  { 
    digitalWrite(2, HIGH);
  }
  currentX = 0;
  moveTo(0,-200);
  if(digitalRead(10))
  {
    digitalWrite(3, HIGH);
  }
  currentY = 0;
  }

void doStep(int dx, int dy)
{
  Serial.println(dx);
  Serial.println(dy);
  if(dx <= 0)
  {
    digitalWrite(dirPinX, LOW);
    dx = -dx;
  }
    else
    {
      digitalWrite(dirPinX, HIGH);
    }

  if(dy <= 0)
  {
    digitalWrite(dirPinY, LOW);
    dy = -dy;
  }
  else
  {
    digitalWrite(dirPinY, HIGH);
  }
  
  for(int i = 0; i < dx; i++)
   {
    digitalWrite(stepPinX, HIGH);
    delayMicroseconds(pulseWidthMicros);
    digitalWrite(stepPinX, LOW);
   
   }
   
  for(int i = 0; i < dy; i++)
   {
    digitalWrite(stepPinY, HIGH);
    delayMicroseconds(pulseWidthMicros);
    digitalWrite(stepPinY, LOW);

   }
}

void Servowrite(int pos)
{
  CPencil.write( pos);
}

void setup() {
    Serial.begin(9600);
    CPencil.attach(A3);
  pinMode(enPin, OUTPUT);
  digitalWrite(enPin, LOW);
//I declare as output the pin of the steps and of the direction for the X axis
  pinMode(dirPinX, OUTPUT);
  pinMode(stepPinX, OUTPUT);
  //I declare as output the pin of the steps and of the direction for the Y axis
  pinMode(dirPinY, OUTPUT);
  pinMode(stepPinY, OUTPUT);  
  //I declare the pin of the limit switch 
   pinMode(9, INPUT_PULLUP);
   pinMode(10, INPUT_PULLUP);
  //autoHome();
}

void loop() {
  moveTo(2000,2000);
  while(digitalRead(9) == LOW)
  {
    digitalWrite(stepPinY, HIGH);
  }
   while(digitalRead(10) == LOW)
  {
    digitalWrite(stepPinX, HIGH);  }  }
