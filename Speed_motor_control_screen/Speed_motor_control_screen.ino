#include <LiquidCrystal.h>
#include "LinearRegression.h"
//Variable Declaration
//current sensor decs
//const int ext_volt = A0;
const int current_vin = A0;           // Analog output of the current sensor
const int avgSamples = 10;            // Number of samples to average the reading over
int sensorValue = 0;                  // Set reading value to 0
float sensitivity = 2.5;    // 100mA per 500mV = 0.2
float Vref = 2500;                     // Output voltage with no current: ~ 2500mV or 2.5V input
float voltage, current;
const int rs = 7, en = 8, d4 = 9, d5 = 10, d6 = 11, d7 = 12;
const int dir = 5, psu = 4;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//encoder and motor decs
int BSet;
int ALastSet = 0;
int BLastSet = 0; //encoder previous state
int angle = 0; //encoder degrees rotated
int startedAngle=0;
byte PinA = 2; //channel a input
byte PinB = 3; //channel b input
int slope = 1;
int Offset_neg = 925;
int ASet;
int Offset_pos = -800;
int force = 0;

//export decs
unsigned long currentMillis = 0;
unsigned long previousMillis = 0;
unsigned long startedMillis = 0;

byte Column1 = 1;
byte Column2 = 2;
byte Column3 = 3;
byte Column4 = 4;
float deltaT = 0;

char startMarker = '<';
char endMarker = '>'; \
char initializeMarker = '^';

bool l;
bool motDir=true;
float testDuration = 1;
bool endType=true;
int curStep=0;
LinearRegression linRegFor=LinearRegression();LinearRegression linRegBack=LinearRegression();


//Initial set up. Loop will only run once
void setup()
{
  pinMode(PinA, INPUT);
  pinMode(PinB, INPUT);
pinMode(dir, OUTPUT);
pinMode(psu, OUTPUT);
digitalWrite(psu, HIGH);
digitalWrite(dir, LOW);

Serial.begin(115200);
  while (!Serial);
  //Serial.print(initializeMarker);   //inializes the entire transmition
  previousMillis = millis();
  ASet = digitalRead(PinA);
  BSet = digitalRead(PinB);
  attachInterrupt(digitalPinToInterrupt(PinA), INCRE, CHANGE);  //used for angle determination
  attachInterrupt(digitalPinToInterrupt(PinB), DECRE, CHANGE);  //used for angle determination
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
//  lcd.print("hi!");
  l=true;
  Serial.print("0 0 0\n");
}

//Continuously running loop
void loop(){
  currentRead();                            //provides current for force measurements
  if(l){
    if(Serial.available()>0){
      char pek = Serial.peek();
      if(pek=='S'){digitalWrite(psu, LOW);digitalWrite(dir, HIGH);exit(0);}
      if(pek=='c'){
        Serial.read();
        callibrateDC();
      }else if(pek=='s'){
        Serial.read();
        Spool();
      }
      else{
        float readIn=Serial.parseFloat();
        switch(curStep){
          case 0:
            motDir=readIn>0;
            break;
          case 1:
            testDuration=readIn;
            break;
          case 2:
            endType=readIn>1.5;
            l=false;      
            startedMillis = millis();
            startedAngle=angle;
            Serial.print(String(motDir)+" "+String(testDuration)+"\n");
            Serial.end();Serial.begin(115200);while (!Serial);Serial.flush();
            previousMillis = millis();
            curStep=-1;
            digitalWrite(psu, (motDir?HIGH:LOW));
            digitalWrite(dir, (motDir?HIGH:LOW));
            break;
          default:
            curStep=-1;
            break;
        }
        curStep++;
      }
    }
  }else{
    char pek = Serial.peek();
    if(pek=='S'){digitalWrite(psu, LOW);digitalWrite(dir, HIGH);exit(0);}
    if(endCondition()){l=true;Serial.print("end\n");digitalWrite(psu, HIGH);digitalWrite(dir, LOW);}else{
      serialOutput();
      currentMillis = millis();
      deltaT = (currentMillis - previousMillis) * .001;
      previousMillis = millis();
    }
  }
  lcdPrint();
  delay(10);
}
void lcdPrint(){
  lcd.begin(16, 2);
  String forc=String(abs(int(Force()))) + String("N");
    String tim=String(int(millis() / 1000)%100) + String("s");
    String ang=String(int(angle / 22)) + String("deg");
    String spac=" ";while(spac.length()+tim.length()+ang.length()<16){spac+=" ";}
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(forc);
    lcd.setCursor(0, 1);
    lcd.print(tim + spac + ang);
    return;
}
//Reads the current and provides an averaged result
int currentRead() {
  for (int i = 0; i < avgSamples; i++)
  {
    sensorValue += analogRead(current_vin);
    delay(5);   // wait 5 milliseconds before the next loop
  }

  //Averaging values
  sensorValue = sensorValue / avgSamples;

  // The on-board ADC is 10-bits -> 2^10 = 1024 -> 5V / 1024 ~= 4.88mV
  // The voltage is in millivolts
  voltage = 4.88 * sensorValue;

  // This will calculate the actual current (in mA)
  // Using the Vref and sensitivity settings you configure
  current = (voltage - Vref) * sensitivity;
  force=slope*current;
//  if ( current > 100 )
//  {
//    force=slope*(current+Offset_pos);
//  }
//  if ( current < -100 )
//    {
//    force=slope*(current+Offset_neg);
//  }
  sensorValue = 0;   // Reset reading value
  return voltage, current, force;
}

//increments the angle when the encoder pins changes values
void INCRE()
{
    if (current < -100){
  ASet = digitalRead(PinA) == HIGH;
  angle += (ASet != BSet) ? -1 : +1;
    }
}
//decrements the angle when the encoder pins changes values
void DECRE()
{
  if (current > 100) {
  BSet = digitalRead(PinB) == HIGH;
  angle += (ASet == BSet) ? -1 : +1;
  }
}
float Force(){
  if(motDir){
    return(linRegFor.calculate(force));
  }else{
    return(linRegBack.calculate(force));
  }
}
//formating for Processing code to convert data into csv file for excel
void serialOutput() {
  Serial.print(Force());
  Serial.print(F(" "));
  Serial.print(angle / 22);
  Serial.print(F(" "));
  Serial.print(millis());
  Serial.print('\n');
}
// Code has encoder, PID, and current sensor parts. need to verify they work as intended. Need to write stopping condition and reversing condition.
bool endCondition(){
  if(endType){return(previousMillis-startedMillis>1000*testDuration);}
  return(previousMillis-startedMillis>(3000*testDuration));
}
void Spool(){
  motDir=Serial.parseInt()>0;
  digitalWrite(psu, (motDir?HIGH:LOW));
  digitalWrite(dir, (motDir?HIGH:LOW));
  while(true){
    if(Serial.available()>0){
      char pek = Serial.read();
      if(pek=='S'){digitalWrite(dir, (!motDir?HIGH:LOW));exit(0);}
      if(pek=='s'){digitalWrite(dir, (!motDir?HIGH:LOW));break;}
    }
  }
  digitalWrite(dir, (!motDir?HIGH:LOW));
}
void callibrateDC(){
  while(Serial.available()<1){delay(1);}
  motDir=Serial.parseInt()>0;
  if(motDir){linRegFor.reset();}else{linRegBack.reset();}
  int l=Serial.parseInt();int dc=0;
  float expecteds[l];
  for(int i=0;i<l;i++){
    while(Serial.available()<=0 || Serial.peek()==0 ){delay(1);}
    expecteds[i]=Serial.parseFloat();
     Serial.println(expecteds[i]); 
  }
  while(dc<l){if(expecteds[dc]==0){dc++;}else{
//    Serial.end();Serial.begin(115200);while(!Serial){delay(1);}Serial.flush();
      float expect=expecteds[dc];Serial.print(expect);
      float result=DC_Weight_Test();
      digitalWrite(dir, (!motDir?HIGH:LOW));
      Serial.print(" ");Serial.print(result);Serial.print("\n");
      if(motDir){linRegFor.learn(result, expect);}else{linRegBack.learn(result, expect);}
      lcd.setCursor(0, 0);
      lcd.print("Weight "+String(dc++)+" Complete");
//      delay(10000);
      if(dc<l){
        Serial.println("Next");
        char pek='0';
        while(pek!='n' && pek!='S'){
          while(Serial.available()<=0 || Serial.peek()==0){delay(1);}
          pek = Serial.read();
          if(pek=='S'){digitalWrite(dir, (!motDir?HIGH:LOW));exit(0);}
        }
      }
  }}
//  lr.Parameters(linRegDC);
//  if(motDir){
//    lr.Parameters(linRegFor);
//  }else{
//    lr.Parameters(linRegBack);
//  }
  double values[3];
  if(motDir){linRegFor.getValues(values);}else{linRegBack.getValues(values);}
  Serial.print(values[0]);Serial.print(" ");Serial.print(values[1]);Serial.print(" ");Serial.print(values[2]);Serial.print("\n");
}
float DC_Weight_Test(){
  digitalWrite(psu, (motDir?HIGH:LOW));
  digitalWrite(dir, (motDir?HIGH:LOW));
//  float v[100];int siz=0;
  float mean=0;int n=0;
  delay(1000);
  unsigned long startingTime=millis();
  while(millis()-startingTime<10000 && n<100){
    lcdPrint();
//     && (abs(v[siz-1]+v[siz-2]+v[siz-3])>10 && abs(v[siz-1]+v[siz-2]+v[siz-3])<3000)
    currentRead();                            //provides current for force measurements
//    v[siz]=force;siz++;
    if((abs(force)>15)){
      n++;mean+=(force-mean)/n;
    }
    delay(10);
  }
  digitalWrite(dir, (!motDir?HIGH:LOW));
  delay(1);
  return(mean);
}
