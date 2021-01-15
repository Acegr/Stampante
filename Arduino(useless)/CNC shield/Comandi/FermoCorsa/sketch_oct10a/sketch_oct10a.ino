void setup() {
  // put your setup code here, to run once:
pinMode(8, OUTPUT);
digitalWrite(8, LOW);
pinMode(5, OUTPUT);
digitalWrite(5, HIGH);
pinMode(2, OUTPUT);
pinMode(9, INPUT_PULLUP);
Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
if(digitalRead(9))
{
  digitalWrite(2, HIGH);
}
else
{
  digitalWrite(2, LOW);
}

  //digitalWrite(2, HIGH);
  delay(1);
  digitalWrite(2, LOW);
  delay(1);

}
