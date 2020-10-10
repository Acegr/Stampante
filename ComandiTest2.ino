//RICORDA CHE L'INPUT DEL FINECORSA E' (INPUT_PULLUP)
//dichiaro la direzione e il pin in cui dare l'impulso per l'asse delle X
byte dirPinX = 5;
byte stepPinX = 2;

//dichiaro la direzione e il pin in cui dare l'impulso per l'asse delle Y
byte dirPinY = 6;
byte stepPinY = 3;

int enPin = 8;
//misure in mm
const float beltXLength = 400.f; 
const float beltYLength = 400.f; 
const float stepLength = 1.f; 

//si devono resettare ad ogni spegnimento
float currentX = 0.f; 
float currentY = 0.f;

//tempo tra un impulso e l'altro
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
  //dichiaro come output il pin dei passi e della direzione per l'asse delle X
  pinMode(dirPinX, OUTPUT);
  pinMode(stepPinX, OUTPUT);
  //dichiaro come output il pin dei passi e della direzione per l'asse delle Y 
  pinMode(dirPinY, OUTPUT);
  pinMode(stepPinY, OUTPUT);  

}

void loop() {
  moveTo(200,200);
  moveTo(0, 0);
}
