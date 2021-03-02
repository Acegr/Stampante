#include <Servo.h>
#include <string.h>
Servo CPencil ;

const unsigned long SheetSizeX = 47670;
const unsigned long SheetSizeY = 33710; 

unsigned long ImageResolutionX;
unsigned long ImageResolutionY;

float StepsPerPixelX;
float StepsPerPixelY;

//I declare the direction and the pin in which to give the impulse for the X axis
byte dirPinX = 5;
byte stepPinX = 2;

int up = 120;
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

void moveTo(float x, float y) //Coordinates are given in engine steps (1 step = 6.2300 μm as measured)
{
  //Using stop switches is too slow (but we should never need these instruction if the program works)
  if(x > SheetSizeX) x = SheetSizeX;
  if(y > SheetSizeY) y = SheetSizeY;
  float xToMove = x - currentX;
  float yToMove = y - currentY;
  doStep(xToMove, yToMove);
  currentX = x;
  currentY = y;
}

void autoHome()
{
  servoWrite(up);
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
  Serial.setTimeout(500);
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

//We use the loop so the board resets every time after we send a Q
void loop() {

  char LastPacket[9] = {}; //For now we only need to know the last packet

  //We wait for an I from the computer to signal the beginning
  while(LastPacket[0] != 'I')
  {
    Serial.readBytes(LastPacket, 9);
  }
  Serial.write("IIIIIIII", 8);
  delay(1000);

  while(LastPacket[0] != 'P')
  {
    memset(LastPacket, 0, 9);
    Serial.readBytes(LastPacket, 9);
  }
  Serial.write("C", 1);
  sscanf(LastPacket+1, "%4X%4X", &ImageResolutionX, &ImageResolutionY);
  StepsPerPixelX = SheetSizeX / ImageResolutionX;
  StepsPerPixelY = SheetSizeY / ImageResolutionY;

  while(LastPacket[0] != 'Q')
  {
    bool Readable = true;
    memset(LastPacket, 0, 9);
    Serial.readBytes(LastPacket, 9);
    bool Readable = true;
    for(int i = 0; int i < 9; i++)
    {
	if(LastPacket[i] == 0)
	{
		Readable = false;
	}
    }
    if(Readable)
    {
      Serial.write("C", 1);
      if(LastPacket[0] == 'M') //moveTo
      {
        servoWrite(up);
        unsigned long targetX = 0;
        unsigned long targetY = 0;
        sscanf(LastPacket+1, "%.4X%.4X", &targetX, &targetY);
        moveTo(targetX*StepsPerPixelX, targetY*StepsPerPixelY);
        servoWrite(down);
      }
      else //Every letter is one instruction
      {
        for(int i = 0; i < 9; i++)
        {
          switch(LastPacket[i])
          {
            case 'U':
            {
              moveTo(currentX, currentY-StepsPerPixelY);
              break;
            }
            case 'D':
            {
              moveTo(currentX, currentY+StepsPerPixelY);
              break;
            }
            case 'R':
            {
              moveTo(currentX+StepsPerPixelX, currentY);
              break;
            }
            case 'L':
            {
              moveTo(currentX-StepsPerPixelX, currentY);
              break;
            }
            default:
            {
              break;
            }
          }
        }
      }
      Serial.write("D", 1);
    }
  }
}
