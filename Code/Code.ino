  /*
  Project ID:     #29AtoMax
  Visual Studio:  1.46.1
  Code Version:   0.4
  Architecture:   8-bit
  Libraries used: 4
  Developer:      Anand P S
  */
  
  #include <Arduino.h>
  #include <HCSR04.h>
  #include <jled.h>
  #include <Wire.h> 
  #include <LiquidCrystal_I2C.h>

  //Calibration Section//
  bool developerMode = 0;
  int presenceSensorFirstTRGValue = 100;
  int presenceSensorSecondTRGValue = 50;
  int handSensorTRGValue = 15;
  int liquidTankHeight = 40;
  int atomizerOnTime = 2000;
  int presenceHoldDelay = 1500;
  int sprayLimit = 2;
  int relayOnPulseLength = 100;
  long startupBacklightHoldDelay = 35000;
  long backlightHoldDelay = 15000;
  
  UltraSonicDistanceSensor presenceSensor(3, 4);
  UltraSonicDistanceSensor handSensor(5, 6);
  UltraSonicDistanceSensor liquidSensor(7, 8);

  #define I2C_ADDR 0x3F //0x27 
  LiquidCrystal_I2C lcd(I2C_ADDR,20,4);
  
  int buzzer = A0,trackingGreenLed = A1,trackingRedLed = A2,bluePowerOnLed = A3;
  int greenDetectionLed = 2,atomizerDriver = 11, redLiquidExhaustedLed = 12,redBootLed = 13; 
  
  auto bluePowerOnLedUpdate = JLed(bluePowerOnLed).Blink(100, 4000).Forever();
  auto greenDetectionLedUpdate = JLed(greenDetectionLed).Blink(800, 1400).Forever();
  auto redLiquidExhaustedLedUpdate = JLed(redLiquidExhaustedLed).Blink(800, 600).Forever();
  auto trackingRedLedUpdate = JLed(trackingRedLed).Blink(600, 400).Forever();
  auto trackingGreenLedUpdate = JLed(trackingGreenLed).Blink(1000, 500).Forever();

  int ps,hs,ls; //Presence sensor, Hand sensor, Liquid Sensor
  int oldL=0;
 
  bool BH=1,PSF=0,ADF=1;//Backlight Hold, Presence Sensor Flag, AtomizerDrive Flag
  int SC=0; //Spray Count
  bool OF = 0; //ON flag
  bool printDisplay = 1; //Display print flag
  
  char a[15] = "Sensors:";
  char b[15] = "Feedback:";
  char c[15] = "Network:";
  char d[15] = "Device Ready!";
  char f[15] = " Failed!";
  char g[20] = "Have a nice day!";
  char p[15] = " Pass";

  unsigned long currentMillis = 0;
  unsigned long systemTriggerMillis = 0;
  unsigned long backlightSleepMillis = 0;
  unsigned long blueLed = 0;
  unsigned long sprayStartTime = 0;
  unsigned long startTime = 0;

void setup () 
{
  //lcd.begin();
  lcd.init();
  lcd.backlight();
  Serial.begin(9600); 

  pinMode(buzzer,OUTPUT);
  pinMode(trackingGreenLed,OUTPUT);
  pinMode(trackingRedLed,OUTPUT);
  pinMode(redBootLed,OUTPUT);
  pinMode(bluePowerOnLed,OUTPUT);
  pinMode(greenDetectionLed,OUTPUT);
  pinMode(redLiquidExhaustedLed,OUTPUT);
  pinMode(atomizerDriver,OUTPUT);

//  while(1)
//  {
//    digitalWrite(atomizerDriver,LOW);
//    lcd.setCursor(5,2);
//    lcd.print("Test Mode!");
//  }
  digitalWrite(atomizerDriver,HIGH);

  if(developerMode==1){
    test();
    while(1);
  }

  boot();
  startUpRun();
}

void loop () 
{
  currentMillis = millis();
  bluePowerOnLedUpdate.Update();

  if(currentMillis>=startupBacklightHoldDelay && BH==1) //Startup backlight holding
  {
    lcd.noBacklight();
    BH = 0;
    Serial.println("Backlight off");
    Serial.println("Sleeping...");
  }

  ps = readPresenceSensor();
  hs = readHandSensor();
  ls = readLiquidSensor();

  // if((oldL-ls)>=2 || (oldL-ls)<=-2){
  //   lcd.setCursor(0,3);
  //   lcd.print("Sanitizer left:");
  //   printPercentage(100,15,3); // printValue, Column, Row
  //   oldL=ls;
  // }

  redLedControl();
  trackingGreenLedControl();
  

  if(ps==2){                  //Presence detection
        
     PSF=1;
     systemTriggerMillis=currentMillis;
  }
  else 
  {
    if((currentMillis-systemTriggerMillis)>=presenceHoldDelay)   // presence Holding delay
    {
      PSF=0;
      ADF=1;
      SC=0;
      printDisplay=1;
    }
  }

  if(PSF==1&&printDisplay==1)
    {
    displayWelcome();
    printDisplay=0;
    }

  if(PSF==1&&hs==1&&ADF==1)
  {
    Serial.println("Hand Detected");
    backlightSleepMillis=millis();
    if(ls>=5){
    spray(millis()); //passing current time
    }
    SC++;  
    if(SC==sprayLimit)
     {
      ADF=0;
     }

   if(ADF==1){
        
     lcd.clear();
     lcd.setCursor(6,1);
     lcd.print("Thank You");
     lcd.setCursor(2,2);
     lcdPrint(g);
   }
   else
   { 
     lcd.clear();
     lcd.setCursor(6,0);
     lcd.print("Thank You");
     digitalWrite(greenDetectionLed,LOW);
     lcd.setCursor(2,1);
     lcdPrint(g);
     
     while(readHandSensor()==1)
     {
       for(int i=0;i<=1;i++)
        {
          digitalWrite(buzzer,HIGH);
          delay(200);
          digitalWrite(buzzer,LOW);
          delay(100);
        }
       int blinkDelay = 300;

       for(int u=0;u<4;u++)
       {
         if(readHandSensor()==0){
           break;
         }
         lcd.setCursor(2,3);
         lcd.print("Cycle Completed!");
         Serial.println("Cycle Completed");
         delay(blinkDelay);
         lcd.setCursor(2,3);
         lcd.print("                ");
         delay(blinkDelay);
       }
     }
   }
     
  }
 
  if(PSF==0&&BH==0&&(currentMillis-backlightSleepMillis)>=backlightHoldDelay)
    {
      lcd.noBacklight();
      digitalWrite(greenDetectionLed,LOW);
      Serial.print("Backlight OFF |");
    }
    else
    {
      lcd.backlight();
      Serial.print("Backlight ON |");
    }

  printValues();   
    
}

void spray(unsigned long sprayStartTime)
{
  digitalWrite(atomizerDriver,LOW);  //Spray ON
  delay(relayOnPulseLength);
  digitalWrite(atomizerDriver,HIGH);
  lcd.clear();

  while(readHandSensor()==1 && millis()-sprayStartTime<=atomizerOnTime)
  {
    for(int c=0; c<20; c++)
    {
      if(readHandSensor()==0){
        break;
      }
      delay(atomizerOnTime/20);
      for(int r=1; r<3; r++)
      {
        lcd.setCursor(c,r);//col,row
        lcd.write(255);
     }
    }
    
  }
  Serial.println("Sprayed");
  digitalWrite(atomizerDriver,LOW);  //Spray OFF
  delay(relayOnPulseLength);
  digitalWrite(atomizerDriver,HIGH);

}
void displayWelcome(){
  Serial.println("Presence Detected");
  for(int i=0;i<=3;i++)
  {
    digitalWrite(greenDetectionLed,LOW);
    delay(100);
    digitalWrite(greenDetectionLed,HIGH);
    delay(60);
  }
  
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(2,0);
  lcd.print("IEDC Masterminds");
  lcd.setCursor(6,1);
  lcd.print("Welcomes");
  lcd.setCursor(7,2);
  lcd.print("You to");
  lcd.setCursor(2,3);
  lcd.print("GPTC Meenangadi.");

  for(int i=0;i<20;i++){
    trackingGreenLedUpdate.Update();
    delay(100);
  }
   

  lcd.clear();
  lcd.blink();
  lcd.setCursor(0,0);
  lcd.print("Insert your Hands");
  lcd.setCursor(0,2);
  lcd.print("System Ready to use");
  lcd.setCursor(0,3);
  lcd.print("AtoMax 1.0.2");

  digitalWrite(buzzer,HIGH);
  delay(200);
  digitalWrite(buzzer,LOW);
  
}
void trackingGreenLedControl(){
  if(ps==2&&ADF==1){
    trackingGreenLedUpdate.Update();
  }
  else{
    digitalWrite(trackingGreenLed,LOW);
  }
}
void redLedControl(){
if(hs==1&&ADF==0){
  redLiquidExhaustedLedUpdate.Update();
  trackingRedLedUpdate.Update();
}
else
{
  digitalWrite(redLiquidExhaustedLed,LOW);
  digitalWrite(trackingRedLed,LOW);
}
}
void printPercentage(int a, int col, int row){
  if(a==100)
  {
    lcd.setCursor(col,row);
    lcd.print(a);
    lcd.print("%");
    
  }
  else if(a<100&&a>9)
  {
    lcd.setCursor(col,row);
    lcd.print(a);
    lcd.print("% ");
    
  }
  else if(a<10)
  {
    lcd.setCursor(col,row);
    lcd.print(a);
    lcd.print("%  "); 
  }
  
}
void printValues(){
  Serial.print("PS: ");
  Serial.print(presenceSensor.measureDistanceCm());
  Serial.print("|HS: ");
  Serial.print(handSensor.measureDistanceCm());
  Serial.print("|LS: ");
  Serial.println(liquidSensor.measureDistanceCm());
}
int readPresenceSensor(){
  int d;
  if(presenceSensor.measureDistanceCm()>presenceSensorFirstTRGValue){
    d=0;
  }
  else if(presenceSensor.measureDistanceCm()>=presenceSensorSecondTRGValue)
  {
    d=1;
  }
  else if(presenceSensor.measureDistanceCm()<presenceSensorSecondTRGValue)
  {
    d=2;
  }
  return d;
}
int readHandSensor(){
  
  int d;
  if(handSensor.measureDistanceCm()<handSensorTRGValue)
  {
    d=1;
  }
  else
  {
    d=0;
  }
  return d;
}
int readLiquidSensor(){
  
  int l;
  l = liquidTankHeight-liquidSensor.measureDistanceCm();
  l = l*100/liquidTankHeight;
  if(l>100)
  {
    l = 100;
  }
  return l;
}
void startUpRun(){
  ls = readLiquidSensor();
  lcd.setCursor(4,1);
  lcd.print("Ready to Use!");
  lcd.setCursor(0,2);
  lcd.print("____________________");
  lcd.setCursor(0,3);
  lcd.print("Sanitizer left:");
  printPercentage(ls,15,3); // printValue, Column, Row
}
void boot(){
 
  int ledOnDelay=500;
  int ledOffDelay=500;

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("+------AtoMax------+");
  lcd.setCursor(0,1);
  lcd.print("|    Powered By    |");
  lcd.setCursor(0,2);
  lcd.print("|   Virtual Logic  |");
  lcd.setCursor(0,3);
  lcd.print("+-IEDC Masterminds-+");

  for(int i=0;i<4;i++){
    digitalWrite(redBootLed,HIGH);
    delay(ledOnDelay);
    digitalWrite(redBootLed,LOW);
    delay(ledOffDelay);
  }
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("----System Check----");
  
  digitalWrite(redBootLed,HIGH);
  delay(1500);
  
  lcd.blink();

  int stepDelay = 200;
  int checkDelay = 1000;
  int delay1 = 225;
  
  lcd.setCursor(0,1);
  lcd.print(a);//Sensor
  delay(checkDelay);
  ps=readPresenceSensor();
  hs=readHandSensor();
  ls=liquidSensor.measureDistanceCm();
  if(ps<400&&ps>0 && hs<400&&hs>0 && ls<400&&ls>0){
    lcdPrint(p);
    ledBlink(stepDelay); 
  }
  else
  {
    lcdPrint(f);
    ledBlink(stepDelay);
  }
  

  lcd.setCursor(0,2);
  lcd.print(b);//Feedback
  delay(checkDelay);
  lcdPrint(p);
  ledBlink(stepDelay);

  lcd.setCursor(0,3);
  lcd.print(c);//Network
  delay(checkDelay);
  lcdPrint(f);
  ledBlink(stepDelay);
  delay(1000);

  spacePrint(0,1,19,3); //startCol,startRow,endCol,endRow

  lcd.setCursor(0,2); 
  delay(checkDelay);  
  lcdPrint(d);     //device ready
  delay(500);

  digitalWrite(redBootLed,LOW);
  digitalWrite(bluePowerOnLed,HIGH);
  delay(500);

  for(int i=0;i<=2;i++)
  {
    digitalWrite(buzzer,HIGH);
    delay(200);
    digitalWrite(buzzer,LOW);
    delay(100);
  }

  lcd.clear();
}
void ledBlink(int Delay){
  for(int i=0;i<=1;i++){
  digitalWrite(redBootLed,LOW);
  delay(Delay);
  digitalWrite(redBootLed,HIGH);
  delay(Delay);
}
}
void lcdPrint(char j[]){
  
  int len = strlen(j);
  
  for(int i=0;i<len;i++)
  {
    lcd.print(j[i]);
    delay(60);
  }
}
void spacePrint(int sc,int sr,int ec,int er){
  
 for(int i=sr;i<=er;i++)
  {
    lcd.noBlink();
    lcd.setCursor(0,i);
    for(int j=sc;j<=ec;j++)
      {
        lcd.print(" ");
      }
  }
}
void test(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("+------AtoMax------+");
  lcd.setCursor(0,1);
  lcd.print("|    Powered By    |");
  lcd.setCursor(0,2);
  lcd.print("|   Virtual Logic  |");
  lcd.setCursor(0,3);
  lcd.print("+-IEDC Masterminds-+");
  Serial.println("+------AtoMax------+");
  Serial.println("|    Powered By    |");
  Serial.println("|   Virtual Logic  |");
  Serial.println("+-IEDC Masterminds-+");

  delay(3500);
  Serial.println("");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Booting into");
  lcd.setCursor(0,1);
  lcd.print("Developer Mode...");                               //Booting...
  Serial.println("Booting into Developer Mode...");
  delay(1500);
  for(int i=0;i<=2;i++)
  {
    digitalWrite(buzzer,HIGH);
    delay(200);
    digitalWrite(buzzer,LOW);
    delay(100);
  }

  lcd.clear();                                              //LED
  lcd.setCursor(0,0);
  lcd.print("Checking LED");
  Serial.println(" ");
  Serial.println("Checking LED");
  delay(1500);

  lcd.clear();                                              //Tracking Green LED
  lcd.print("Green LED: ON");
  Serial.println("Tracking Green LED: ON");
  digitalWrite(trackingGreenLed,HIGH);
  delay(2000);
  lcd.clear();
  lcd.print("Green LED: OFF");
  Serial.println("Tracking Green LED: OFF");
  digitalWrite(trackingGreenLed,LOW);
  delay(1000);

  lcd.clear();                                              //Tracking RED LED
  lcd.print("RED LED: ON");
  Serial.println("");
  Serial.println("Tracking RED LED: ON");
  digitalWrite(trackingRedLed,HIGH);
  delay(2000);
  lcd.clear();
  lcd.print("RED LED: OFF");
  Serial.println("Tracking RED LED: OFF");
  digitalWrite(trackingRedLed,LOW);
  delay(1000);

  lcd.clear();                                              //RED BOOT LED
  lcd.print("RED BOOT LED: ON");
  Serial.println("");
  Serial.println("RED BOOT LED: ON");
  digitalWrite(redBootLed,HIGH);
  delay(2000);
  lcd.clear();
  lcd.print("RED BOOT LED: OFF");
  Serial.println("RED BOOT LED: OFF");
  digitalWrite(redBootLed,LOW);
  delay(1000);

  lcd.clear();                                              //Power ON Blue LED
  lcd.print("BLUE LED: ON");
  Serial.println("");
  Serial.println("Power ON Blue LED: ON");
  digitalWrite(bluePowerOnLed,HIGH);
  delay(2000);
  lcd.clear();
  lcd.print("BLUE LED: OFF");
  Serial.println("Power ON Blue LED: OFF");
  digitalWrite(bluePowerOnLed,LOW);
  delay(1000);

  lcd.clear();                                              //Green Detection LED
  lcd.print("Green DET LED: ON");
  Serial.println("");
  Serial.println("Green Detection LED: ON");
  digitalWrite(greenDetectionLed,HIGH);
  delay(2000);
  lcd.clear();
  lcd.print("Green DET LED: OFF");
  Serial.println("Green Detection LED: OFF");
  digitalWrite(greenDetectionLed,LOW);
  delay(1000);

  lcd.clear();                                              //Red Liquid Exhausted LED
  lcd.print("RED Exhaust LED: ON");
  Serial.println("");
  Serial.println("Red Liquid Exhausted LED: ON");
  digitalWrite(redLiquidExhaustedLed,HIGH);
  delay(2000);
  lcd.clear();
  lcd.print("RED Exhaust LED: OFF");
  Serial.println("Red Liquid Exhausted LED: OFF");
  digitalWrite(redLiquidExhaustedLed,LOW);
  delay(1000);


  lcd.clear();                                            //Cheching LCD Backlight
  lcd.setCursor(0,0);
  lcd.print("CHK LCD Backlight");
  Serial.println(" ");
  Serial.println("Cheching LCD Backlight");
  delay(1500);

  
  lcd.clear();                                              
  lcd.print("LCD Backlight: OFF");
  Serial.println("");
  Serial.println("LCD Backlight: OFF");
  delay(1000);
  lcd.noBacklight();
  delay(1000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("LCD Backlight: ON");
  Serial.println("LCD Backlight: ON");
  lcd.backlight();
  delay(1000);
 
                                                            //Atomizer
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Checking Atomizer");
  Serial.println(" ");
  Serial.println("Checking Atomizer");
  delay(1500);
  lcd.clear();
  lcd.print("Atomizer: ON");
  Serial.println("Atomizer: ON");
  digitalWrite(atomizerDriver,LOW);
  delay(100);
  digitalWrite(atomizerDriver,HIGH);
  delay(2000);
  lcd.clear();
  lcd.print("Atomizer: OFF");
  Serial.println("Atomizer: OFF");
  digitalWrite(atomizerDriver,LOW);
  delay(100);
  digitalWrite(atomizerDriver,HIGH);
  delay(2000);
while(1){
  lcd.clear();                                                  //Presence Sensor
  lcd.setCursor(0,0);
  lcd.print("Checking..");
  lcd.setCursor(0,1);
  lcd.print("presence Sensor");
  Serial.println("");
  Serial.println("Checking..");
  Serial.println("presence Sensor");
  delay(1000);
  startTime=millis();
  while((millis()-startTime)<=5000){
    int pSense=presenceSensor.measureDistanceCm();
  lcd.setCursor(0,3);
  lcd.print("P Value: ");
  lcd.setCursor(9,3);
  lcd.print(pSense);
  lcd.print("cm   ");
  Serial.print("P Value: ");
  Serial.print(pSense);
  Serial.println(" cm");
  delay(100);
  }

  lcd.clear();                                                  //Hand Sensor
  lcd.setCursor(0,0);
  lcd.print("Checking..");
  lcd.setCursor(0,1);
  lcd.print("Hand Sensor");
  Serial.println("");
  Serial.println("Checking..");
  Serial.println("Hand Sensor");
  delay(1000);
  startTime=millis();
  while((millis()-startTime)<=5000){
    int hSense=handSensor.measureDistanceCm();
  lcd.setCursor(0,3);
  lcd.print("H Value: ");
  lcd.setCursor(9,3);
  lcd.print(hSense);
  lcd.print("cm   ");
  Serial.print("H Value: ");
  Serial.print(hSense);
  Serial.println(" cm");
  delay(100);
  }

  lcd.clear();                                                  //Liquid Sensor
  lcd.setCursor(0,0);
  lcd.print("Checking..");
  lcd.setCursor(0,1);
  lcd.print("Liquid Sensor");
  Serial.println("");
  Serial.println("Checking..");
  Serial.println("Liquid Sensor");
  delay(1000);
  startTime=millis();
  while((millis()-startTime)<=5000){
    int lSense=liquidSensor.measureDistanceCm();
  lcd.setCursor(0,3);
  lcd.print("L Value: ");
  lcd.setCursor(9,3);
  lcd.print(lSense);
  lcd.print("cm   ");
  Serial.print("L Value: ");
  Serial.print(lSense);
  Serial.println(" cm");
  delay(100);
  }
 }
}
