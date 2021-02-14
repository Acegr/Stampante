#include <Servo.h>
//REMEMBER, THE INPUT OF THE LIMIT SWITCH IS: (INPUT_PULLUP)
//I declare the servo's name
Servo CPencil;
bool StepX = true;
bool Start = false;
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
<<<<<<< HEAD:TestComandi/Commands_15-01-2021/Commands_15-01-2021.ino
int microseconds = 600;
=======
int microseconds = 100;  
>>>>>>> 7fdd78a55caaf66e698bdb724bf7cc888081b7ff:TestComandi/Commands_15-01-2021.ino

unsigned long numberOfSteps = 0;

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
  moveTo(-200000, -200000);
}

void doStep(long dx, long dy)
{

  if (dx <= 0)
  {
    digitalWrite(dirPinX, LOW);
    dx = -dx;
  }
  else
  {
    digitalWrite(dirPinX, HIGH);
  }

  if (dy <= 0)
  {
    digitalWrite(dirPinY, LOW);
    dy = -dy;
  }
  else
  {
    digitalWrite(dirPinY, HIGH);
  }

  for (int i = 0; i < dx; i++)
  {
    if (digitalRead(9) == LOW) break;
    digitalWrite(stepPinX, HIGH);
    delayMicroseconds(microseconds);
    digitalWrite(stepPinX, LOW);
<<<<<<< HEAD:TestComandi/Commands_15-01-2021/Commands_15-01-2021.ino
=======
   
   }
   
  for(int i = 0; i < dy; i++)
   {
    digitalWrite(stepPinY, HIGH);
    delayMicroseconds(microseconds);
    digitalWrite(stepPinY, LOW);

   }
}
>>>>>>> 7fdd78a55caaf66e698bdb724bf7cc888081b7ff:TestComandi/Commands_15-01-2021.ino

    for (int i = 0; i < dy; i++)
    {
      if (digitalRead(10) == LOW) 
      {
        currentY = 0;
        break;
      }
      break;
      digitalWrite(stepPinY, HIGH);
      delayMicroseconds(microseconds);
      digitalWrite(stepPinY, LOW);

    }
  }
}

  void Servowrite(int pos)
  {
    CPencil.write( pos);
  }

  void setup () {
    Serial.begin(9600);
    CPencil.attach(A3);
    delay(5000);
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

  void loop () {
moveTo(1,1000);
    /*
      }
      if(digitalRead(9) == LOW)
      {
      StepX = false;
      Serial.println(numberOfSteps);
      }
      else StepX = true;
      if(StepX)
      {
      digitalWrite(stepPinX, HIGH);
      delayMicroseconds(100);
      digitalWrite(stepPinX, LOW);
      numberOfSteps++;
      }

      if(digitalRead(10)== LOW)
      {
      digitalWrite(stepPinX, HIGH);
      }
      else
      {
      digitalWrite(stepPinX, LOW);
      }*/
  }
