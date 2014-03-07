#include <JoystickShield.h>
#include <Servo.h>

/*
 Joystick Shield Sample Sketch
 
 Reads the buttons and joystick position of SparkFun's Arduino
 Joystick Shield. The Joystick Shield is available from SparkFun's
 website:
 
 http://www.sparkfun.com/commerce/product_info.php?products_id=9490 
 
 created Nov. 23, 2009
 by Ryan Owens
*/

//Create variables for each button on the Joystick Shield to assign the pin numbers
char button0=3, button1=4, button2=5, button3=6;
char sel=2;

Servo eyesLR;  // servo for eyes left right
Servo eyesUD;  // servo for eyes up down
Servo mouth;
unsigned long time;
int previousState=0;
int mouthValue=80;
int joystickX;
int joystickY;
int currentX;
int currentY;
JoystickShield joystickShield; // create an instance of JoystickShield object
String command;
String lastCommand;

void setup(void)
{
  pinMode(sel, INPUT);      //Set the Joystick 'Select'button as an input
  digitalWrite(sel, HIGH);  //Enable the pull-up resistor on the select button
  
  pinMode(button0, INPUT);      //Set the Joystick button 0 as an input
  digitalWrite(button0, HIGH);  //Enable the pull-up resistor on button 0

  pinMode(button1, INPUT);      //Set the Joystick button 1 as an input
  digitalWrite(button1, HIGH);  //Enable the pull-up resistor on button 1
  
  pinMode(button2, INPUT);      //Set the Joystick button 2 as an input
  digitalWrite(button2, HIGH);  //Enable the pull-up resistor on button 2

  pinMode(button3, INPUT);      //Set the Joystick button 3 as an input
  digitalWrite(button3, HIGH);  //Enable the pull-up resistor on button 3
  
  eyesLR.attach(16);  // attaches the servo on pin 9 to the servo object 
  eyesUD.attach(17);  // attaches the servo on pin 9 to the servo object 
  mouth.attach(18);  // attfches the servo on pin 9 to the servo object (mouth closed = 90 degrees, open = 60)

  command="";
  
  Serial.begin(9600);           //Turn on the Serial Port at 9600 bps
  
  joystickShield.calibrateJoystick();
  Serial.print("starting");
}


void loop(void)
{

  joystickX=analogRead(0);
  joystickY=analogRead(1);

  if( joystickY > 800 ){
      time=millis();
      Serial.print("eyesYup");
      Serial.print(",");
      Serial.print(time);
      Serial.print("\n");      
      while(joystickY > 800 ){
       eyesUD.write(160);
       joystickY=analogRead(1);
       delay(10);
      }
      time=millis();
      Serial.print("eyesYreturn");
      eyesUD.write(90);
      Serial.print(",");
      Serial.print(time);
      Serial.print("\n");      
  }
  if( joystickY < 400 ){
      time=millis();
      Serial.print("eyesYdown");
      Serial.print(",");
      Serial.print(time);
      Serial.print("\n");      
      while(joystickY < 400 ){
       eyesUD.write(45);
       joystickY=analogRead(1);
       delay(10);
      }
      time=millis();
      Serial.print("eyesYreturn");
      eyesUD.write(90);
      Serial.print(",");
      Serial.print(time);
      Serial.print("\n");      
  }  
  if( joystickX < 400 ){
      time=millis();
      Serial.print("eyesXleft");
      Serial.print(",");
      Serial.print(time);
      Serial.print("\n");      
      while(joystickX < 400 ){
       eyesLR.write(160);
       joystickX=analogRead(0);
       delay(10);
      }
      time=millis();
      Serial.print("eyesXreturn");
      eyesLR.write(90);
      Serial.print(",");
      Serial.print(time);
      Serial.print("\n");      
  }
  if( joystickX > 800 ){
      time=millis();
      Serial.print("eyesXright");
      Serial.print(",");
      Serial.print(time);
      Serial.print("\n");      
      while(joystickX > 800 ){
       eyesLR.write(45);
       joystickX=analogRead(0);
       delay(10);
      }
      time=millis();
      Serial.print("eyesXreturn");
      eyesLR.write(90);
      Serial.print(",");
      Serial.print(time);
      Serial.print("\n");      
  }    
  
    if( digitalRead(button3) == LOW ){
      time=millis();
      Serial.print("mouthOpen");
      Serial.print(",");
      Serial.print(time);
      Serial.print("\n");      
      while( digitalRead(button3) == LOW ){
       mouth.write(50);

       delay(10);
      }
      time=millis();
      Serial.print("mouthClose");
      mouth.write(80);
      Serial.print(",");
      Serial.print(time);
      Serial.print("\n");      
  }   

  if(digitalRead(button3) == LOW){
    if(mouthValue!=50){
      mouth.write(50);
    }
    if(previousState==0){
      time=millis();
      Serial.print("low");
      Serial.print(",");
      Serial.print(time);
      Serial.print("\n");      
    }
  }
  
  //Wait for 100 ms, then go back to the beginning of 'loop' and repeat.
  delay(10);
}

void up() {
    command="up";
    if(lastCommand!="up"){
      Serial.println("Up from callback,");
      time=millis();
      Serial.println(time);
      Serial.println("\n");
      //lastCommand="up";
    }
    eyesUD.write(160);
}

void rightUp() {
    command="rightUp";
    if(lastCommand!="rightUp") {
      Serial.println("Right Up from callback,");
      time=millis();
      Serial.println(time);
      Serial.println("\n");
    }
    eyesLR.write(60);
    eyesUD.write(160);
}

void right() {
    command="right";
    if(lastCommand!="right"){
      Serial.println("Right from callback,");
      time=millis();
      Serial.println(time);
      Serial.println("\n");
    }
    eyesLR.write(60);
}

void rightDown() {
    if(lastCommand!="rightDown"){
      Serial.println("Right Down from callback,");
      time=millis();
      Serial.println(time);
      Serial.println("\n");
    }
    eyesLR.write(60);
    eyesUD.write(45);        
}

void down() {
    if(lastCommand!="down"){
      Serial.println("Down from callback,");
      time=millis();
      Serial.println(time);
      Serial.println("\n");
    }
    eyesUD.write(45);
}

void leftDown() {
    if(lastCommand!="leftDown"){
      Serial.println("Left Down from callback,");
      time=millis();
      Serial.println(time);
      Serial.println("\n");
    }
    eyesLR.write(160);
    eyesUD.write(45);         
}

void left() {
    if(lastCommand!="left"){
      Serial.println("Left from callback,");
      time=millis();
      Serial.println(time);
      Serial.println("\n");
    }
    eyesLR.write(120);
}

void leftUp() {
    if(lastCommand!="leftUp"){
      Serial.println("Left Up from callback,");
      time=millis();
      Serial.println(time);
      Serial.println("\n");
    }
    eyesLR.write(160);
    eyesUD.write(160);     
}


void joystickButton() {
    Serial.println("Joystick from callback,");
        time=millis();
    Serial.println(time);
    Serial.println("\n");
}

void upButton() {
    Serial.println("Up Button from callback,");
        time=millis();
    Serial.println(time);
    Serial.println("\n");
}

void rightButton() {
    Serial.println("Right Button from callback");
}

void downButton() {
    Serial.println("Down Button from callback");
}

void leftButton() {
    if(lastCommand!="mouth"){
      Serial.println("mouth,");
      time=millis();
      Serial.println(time);
      Serial.println("\n");
    }
    mouth.write(60);
}

void FButton() {
    Serial.println("F Button from callback");
}

void EButton() {
    Serial.println("E Button from callback");
}

ff
