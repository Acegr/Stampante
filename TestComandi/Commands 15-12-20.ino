//REMEBER THE INPUT OF THE LIMIT SWITCH IS: (INPUT_PULLUP)
//I declare the direction and the pin in which to give the impulse for the X axis
byte dirPinX = 5;
byte stepPinX = 2;

//I declare the direction and the pin in which to give the impulse for the Y axis
byte dirPinY = 6;
byte stepPinY = 3;

int enPin = 8;
//measures in mm
const float beltXLength = 400.f; 
const float beltYLength = 400.f; 
const float stepLength = 1.f; 

//they must be resetted at every turn off
float currentX = 0.f; 
float currentY = 0.f;

//time between two pulses
int pulseWidthMicros = 250;  
int millisbetweenSteps = 10;

void moveTo(float x, float y)
{
  float xToMove = x - currentX;
  float yToMove = y - currentY;
  doStep(xToMove / stepLength, yToMove / stepLength);
  currentX = x;
  currentY = y;
}

void autoHome()
{
  moveTo(-200 , -200);
  if(digitalRead(9))
  { 
    digitalWrite(2, HIGH);
  }
  currentX = 0;
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
    delay(millisbetweenSteps);
   }
   
  for(int i = 0; i < dy; i++)
   {
    digitalWrite(stepPinY, HIGH);
    delayMicroseconds(pulseWidthMicros);
    digitalWrite(stepPinY, LOW);
    delay(millisbetweenSteps);
   }
}



void setup() {
    Serial.begin(9600);
    
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
 
  autoHome();
}

void loop() {
  moveTo(200,200);
  moveTo(0, 0);
}
