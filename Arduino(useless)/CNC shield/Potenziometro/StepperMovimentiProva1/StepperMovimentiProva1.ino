int velocity;
const int pot1 = A0;
const int pot2  = A1;
int potVal1;
int potVal2;
int velocity1;
int velocity2;
void setup() {
  // put your setup code here, to run once:
Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
potVal1 = analogRead(pot1);
potVal2 = analogRead(pot2);
velocity1 = map(potVal1, 0, 1023, 0, 100);
velocity2 = map(potVal2, 0, 1023, 0, 100);

while (1 > 0)
{
  digitalWrite(pot1, HIGH);
  delay(100+ velocity1);
  Serial.print(potVal1);
}

while (1 > 0)
{
  digitalWrite(pot2, HIGH);
  delay(100+ velocity2);
  Serial.print(potVal2);
}


}
