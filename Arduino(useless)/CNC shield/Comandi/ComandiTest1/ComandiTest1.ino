//dichiaro la direzione e il pin in cui dare l'impulso per l'asse delle X
byte dirPinX = 5;
byte stepPinX = 2;
//dichiaro la direzione e il pin in cui dare l'impulso per l'asse delle Y
byte dirPinY = 6;
byte stepPinY = 3;
//dichiaro il numero di passi e il tempo che deve passare prima che cambi stato
int numberOfSteps = 1;
int pulseWidthMicros = 250;  
int millisbetweenSteps = 10; 
//dichiaro il pin per abilitare gli stepper (SEMPRE IL NUMERO 8)
int enPin = 8;
void setup() {
//inizio la comunicazione 
  Serial.begin(9600);
  //dichiaro come output il pin 8 e abilito gli stepper assegnandoli lo stato LOW
  pinMode(enPin, OUTPUT);
  digitalWrite(enPin, LOW);
  //dichiaro come output il pin dei passi e della direzione per l'asse delle X
  pinMode(dirPinX, OUTPUT);
  pinMode(stepPinX, OUTPUT);
  //dichiaro come output il pin dei passi e della direzione per l'asse delle Y 
  pinMode(dirPinY, OUTPUT);
  pinMode(stepPinY, OUTPUT);  
  //assegno lo stato HIGH alle direzioni dei pin (HIGH antiorario, LOW orario)
  digitalWrite(dirPinX, HIGH);
  digitalWrite(dirPinY, HIGH);
  

  
}
void loop()
{
  //creo un loop in cui gli stati dello stepper si alternano per creare movimento (asse X) 
     for(int n = 0; n < numberOfSteps; n++) 
  {
    digitalWrite(stepPinX, HIGH);
    delayMicroseconds(pulseWidthMicros);
    digitalWrite(stepPinX, LOW);
    delay(millisbetweenSteps);
  }
  delay(1000);
  //creo un loop in cui gli stati dello stepper si alternano per creare movimento (asse Y)
     for(int n = 0; n < numberOfSteps; n++) 
  {
    digitalWrite(stepPinY, HIGH);
    delayMicroseconds(pulseWidthMicros);
    digitalWrite(stepPinY, LOW);
    delay(millisbetweenSteps);
  }
  delay(1000);

}
  
