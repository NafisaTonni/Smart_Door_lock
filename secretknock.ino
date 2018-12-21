#include <Servo.h>// for servo motor
Servo myservo; //servo function set krci
#include <SoftwareSerial.h>// sim er jonno
SoftwareSerial SIM900A(10,11);// sim port rx and tx set krci
    

 
// Pin definitions

const int knockSensor = A9;         // Piezo sensor arduino r A0 pin e set kora
const int programSwitch = 51;       // If this is high we program a new code.
const int buzzer = 8;              // Status LED
const int greenLED =0;            // Status LED

const int threshold = 1000;           // Minimum signal from the piezo 
const int rejectValue = 25;        // If an individual knock don't unlock..
const int averageRejectValue = 15; // If the average timing of the knocks don't unlock.
const int knockFadeTime = 150;     // milliseconds  listen for another one.
const int lockTurnTime = 6500;      // milliseconds that we run the motor to get it to go a half turn.

const int maximumKnocks = 20;       // Maximum number of knocks to listen for.
const int knockComplete = 1200;     // Longest time to wait for a knock before we assume that it's finished.


// Variables.
 int wrongcount=0;
int secretCode[maximumKnocks] = {50, 25, 25, 50, 100, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // Initial setup: "Shave and a Hair Cut, two bits."
int knockReadings[maximumKnocks];   // When someone knocks this array fills with delays between knocks.
int knockSensorValue = 0;           // Last reading of the knock sensor.
int programButtonPressed = false;   // Flag so we remember the programming button setting at the end of the cycle.

void setup() {
 
  myservo.attach(9);
  myservo.write(0);
  pinMode(buzzer, OUTPUT);
  pinMode(greenLED, OUTPUT);
//  pinMode(programSwitch, INPUT);
  Serial.begin(9600);               			// Uncomment the Serial.bla lines for debugging.
  Serial.println("Program start.");  			// but feel free to comment them out after it's working right.
  SIM900A.begin(9600);   // Setting the baud rate of GSM Module  
  Serial.println ("SIM900A Ready");
 
}

void loop() {

  // Listen for any knock at all.
  knockSensorValue = analogRead(knockSensor);
  
  if (digitalRead(programSwitch)==LOW){  // is the program button pressed?
    programButtonPressed = true;          // Yes, so lets save that state
    digitalWrite(buzzer, HIGH);           // and turn on the red light too so we know we're programming.
  } else {
    programButtonPressed = false;
    digitalWrite(buzzer, LOW);
  }
  if (knockSensorValue >=threshold){
    listenToSecretKnock();
  }





  //////for sim

  if( Serial.available()>0){
    RecieveMessage();
    
  }

    }

    
// Records the timing of knocks.
void listenToSecretKnock(){
  Serial.println("knock starting");   

  int i = 0;
  // First lets reset the listening array.
  for (i=0;i<maximumKnocks;i++){
    knockReadings[i]=0;
  }
  
  int currentKnockNumber=0;         			// Incrementer for the array.
  int startTime=millis();           			// Reference for when this knock started.
  int now=millis();
  
  digitalWrite(greenLED, LOW);      			// we blink the LED for a bit as a visual indicator of the knock.
  if (programButtonPressed==true){
     digitalWrite(buzzer, LOW);                         // and the red one too if we're programming a new knock.
  }
  delay(knockFadeTime);                       	        // wait for this peak to fade before we listen to the next one.
  digitalWrite(greenLED, HIGH);  
  if (programButtonPressed==true){
     digitalWrite(buzzer, HIGH);                        
  }
  do {
    //listen for the next knock or wait for it to timeout. 
    knockSensorValue = analogRead(knockSensor);
    if (knockSensorValue >=threshold){                   //got another knock...
      //record the delay time.
      Serial.println("knock.");
      now=millis();
      knockReadings[currentKnockNumber] = now-startTime;
      currentKnockNumber ++;                             //increment the counter
      startTime=now;          
      // and reset our timer for the next knock
      digitalWrite(greenLED, LOW);  
      if (programButtonPressed==true){
        digitalWrite(buzzer, LOW);                       // and the red one too if we're programming a new knock.
      }
      delay(knockFadeTime);                              // again, a little delay to let the knock decay.
    //  digitalWrite(greenLED, HIGH);
      if (programButtonPressed==true){
        digitalWrite(buzzer, HIGH);                         
      }
    }
    int wrongknock=0;
    now=millis();
    
    //did we timeout or run out of knocks?
  } while ((now-startTime < knockComplete) && (currentKnockNumber < maximumKnocks));
  
  //we've got our knock recorded, lets see if it's valid
  if (programButtonPressed==false){             // only if we're not in progrmaing mode.
    if (validateKnock() == true){
      wrongcount=0;
      triggerDoorUnlock(); 
    } else {
      wrongcount++;
      Serial.println("Secret knock failed.");
      Serial.println(wrongcount);
        switch(wrongcount)
      {
        case 3:
          SendMessage();
          wrongcount=0;
         // break;
        default:
          RecieveMessage();
          break;
      }
    //  digitalWrite(greenLED, LOW);  		// We didn't unlock, so blink the red LED as visual feedback.
      for (i=0;i<4;i++){					
        digitalWrite(buzzer, HIGH);
        delay(100);
        digitalWrite(buzzer, LOW);
        delay(100);
      }
    //  digitalWrite(greenLED, HIGH);
    }
  } else { // if we're in programming mode we still validate the lock, we just don't do anything with the lock
    validateKnock();
    // and we blink the green and red alternately to show that program is complete.
    Serial.println("New lock stored.");
    digitalWrite(buzzer, LOW);
    digitalWrite(greenLED, HIGH);
    for (i=0;i<3;i++){
      delay(100);
      digitalWrite(buzzer, HIGH);
      digitalWrite(greenLED, LOW);
      delay(100);
      digitalWrite(buzzer, LOW);
      digitalWrite(greenLED, HIGH);      
    }
  }
}


// Runs the motor (or whatever) to unlock the door.
void triggerDoorUnlock(){
  Serial.println("Door unlocked!");
  int i=0;
  myservo.write(90);
  digitalWrite(greenLED, HIGH);            // And the green LED too.
  
  delay (lockTurnTime);                    // Wait a bit.
  myservo.write(0);
   
}

// Sees if our knock matches the secret.
// returns true if it's a good knock, false if it's not.
// todo: break it into smaller functions for readability.
boolean validateKnock(){
  int i=0;
 
  // simplest check first: Did we get the right number of knocks?
  int currentKnockCount = 0;
  int secretKnockCount = 0;
  int maxKnockInterval = 0;          			// We use this later to normalize the times.
  
  for (i=0;i<maximumKnocks;i++){
    if (knockReadings[i] > 0){
      currentKnockCount++;
    }
    if (secretCode[i] > 0){  					//todo: precalculate this.
      secretKnockCount++;
    }
    
    if (knockReadings[i] > maxKnockInterval){ 	// collect normalization data while we're looping.
      maxKnockInterval = knockReadings[i];
    }
  }
  
  // If we're recording a new knock, save the info and get out of here.
  if (programButtonPressed==true){
      for (i=0;i<maximumKnocks;i++){ // normalize the times
        secretCode[i]= map(knockReadings[i],0, maxKnockInterval, 0, 100); 
      }
      // And flash the lights in the recorded pattern to let us know it's been programmed.
      digitalWrite(greenLED, LOW);
      digitalWrite(buzzer, LOW);
      delay(1000);
      digitalWrite(greenLED, HIGH);
      digitalWrite(buzzer, HIGH);
      delay(50);
      for (i = 0; i < maximumKnocks ; i++){
        digitalWrite(greenLED, LOW);
        digitalWrite(buzzer, LOW);  
        // only turn it on if there's a delay
        if (secretCode[i] > 0){                                   
          delay( map(secretCode[i],0, 100, 0, maxKnockInterval)); // Expand the time back out to what it was.  Roughly. 
          digitalWrite(greenLED, HIGH);
          digitalWrite(buzzer, HIGH);
        }
        delay(50);
      }
	  return false; 	// We don't unlock the door when we are recording a new knock.
  }
  
  if (currentKnockCount != secretKnockCount){
    return false; 
  }
  
  /*  compare the difference of knocks, not the absolute time between them.
        */
  int totaltimeDifferences=0;
  int timeDiff=0;
  for (i=0;i<maximumKnocks;i++){ // Normalize the times
    knockReadings[i]= map(knockReadings[i],0, maxKnockInterval, 0, 100);      
    timeDiff = abs(knockReadings[i]-secretCode[i]);
    if (timeDiff > rejectValue){ // Individual value too far out of whack
      return false;
    }
    totaltimeDifferences += timeDiff;
  }
  // It can also fail if the whole thing is too inaccurate.
  if (totaltimeDifferences/secretKnockCount>averageRejectValue){
    return false; 
  }
  
  return true;
  
}



//////////////////////////////////////////////    sim functions//////////////////////////////////////////////


  void SendMessage()
    {
      Serial.println ("Sending Message");
      SIM900A.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
      delay(1000);
      Serial.println ("Set SMS Number");
      SIM900A.println("AT+CMGS=\"+8801991948974\"\r"); //Mobile phone number to send message
      delay(1000);
      Serial.println ("Set SMS Content");
      SIM900A.println("Someone trying to unlock");// Messsage content
      delay(100);
      Serial.println ("Finish");
      SIM900A.println((char)26);// ASCII code of CTRL+Z
      delay(1000);
      Serial.println ("Message has been sent ->SMS Selete");
    }
     void RecieveMessage()
    {
      Serial.println ("SMS");
      delay (1000);
      SIM900A.println("AT+CNMI=2,2,0,0,0");// AT Command to receive a live SMS
      delay(1000);
      Serial.write ("Unread Message done");
     }
