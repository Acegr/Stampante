#include <Servo.h>
Servo CPencil ;
//I declare the direction and the pin in which to give the impulse for the X axis
byte dirPinX = 5;
byte stepPinX = 2;

int up = 180;
int down = 40;

//I declare the direction and the pin in which to give the impulse for the Y axis
byte dirPinY = 6;
byte stepPinY = 3;

int enPin = 8;

//they must be resetted at every turn off
float currentX = 0.f;
float currentY = 0.f;

//time between two pulses
int microseconds = 600;
 void servoWrite(int pos)
  {
    CPencil.write(pos);
  }

void doStep(long dx, long dy)
{

  if (dx < 0)
  {
    digitalWrite(dirPinX, LOW);
    dx = -dx;
  }
  else
  {
    digitalWrite(dirPinX, HIGH);
  }

  if (dy < 0)
  {
    digitalWrite(dirPinY, LOW);
    dy = -dy;
  }
  else
  {
    digitalWrite(dirPinY, HIGH);
  }

  for (long i = 0; i < dx; i++)
  {
    digitalWrite(stepPinX, HIGH);
    delayMicroseconds(microseconds);
    digitalWrite(stepPinX, LOW);
     }
  for (long i = 0; i < dy; i++)
    {
      digitalWrite(stepPinY, HIGH);
      delayMicroseconds(microseconds);
      digitalWrite(stepPinY, LOW);
    }
}

void moveTo(float x, float y)
{
  float xToMove = x - currentX;
  float yToMove = y - currentY;
  doStep(xToMove - currentX, yToMove - currentY);
  currentX = x;
  if (currentX < 0) currentX = 0;
  currentY = y;
  if (currentY < 0) currentY = 0;
}

void autoHome()
{
  digitalWrite(dirPinX, LOW);
  digitalWrite(dirPinY,LOW);
  //x motor
  for (long i = 0; i < 200000; i++)
  {
    if (digitalRead(9) == LOW)
    {
      currentX = 0;
      break;
    }
    digitalWrite(stepPinX, HIGH);
    delayMicroseconds(microseconds);
    digitalWrite(stepPinX, LOW);
  }
  //Y motor
  for (long i = 0; i < 200000; i++)
  {
    if (digitalRead(10) == LOW)
    {
      currentY = 0;
      break;
    }
    digitalWrite(stepPinY, HIGH);
    delayMicroseconds(microseconds);
    digitalWrite(stepPinY, LOW);
  }
}
void setup() {
  Serial.begin(9600);
  CPencil.attach(4);
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
}

void loop() {
  servoWrite(up);
  
}
