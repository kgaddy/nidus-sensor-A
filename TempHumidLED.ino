/*
  The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD R/W pin to Ground 
 * LCD VO pin (pin 3) to PWM pin 9
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2

 */
#include <SPI.h>
#include <Ethernet.h>
// include the library code:
#include <LiquidCrystal.h>
#include<stdlib.h>

// I2C library
#include <Wire.h>
// Sensor I2C address
// Data pin from sensor
const byte ADDRESS = 0x27;
const int  CLOCK_PIN = 5;
const int  DATA_PIN = 4;
byte mac[] = {  
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 
  192,168,1,177 };//this ipaddress
byte server[]   = {
  192,168,1,2};//the node server
EthernetClient client;

String varOne,contentLen,length;

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

void setup() {
  //LCD
  // declare pin 9 to be an output:
  pinMode(9, OUTPUT);  
  analogWrite(9, 100);   

  
  Ethernet.begin(mac, ip);
  //TEMPHUMID
    // Start serial and 1-wire communication
  Serial.begin( 9600 );
  Wire.begin();
   
  // Turn on the HIH6130 sensor 
  pinMode( DATA_PIN, OUTPUT );
  digitalWrite( DATA_PIN, HIGH ); 
  
  // Give sensor time to start up 
  delay( 5000 );
}

void loop() {
  
  byte state;
  float humidity;
  float temperature;
     
  // Read the values from the sensor
  // Takes local values to place sensor values
  // Returns sensor state
  state = getTemperatureHumidity( humidity, temperature );
    // Report on sensor state    
  switch( state )
  {
    case 0:  
      Serial.print( "normal" );
      break;
    case 1:  
      Serial.print( "stale data" );
      break;
    case 2:  
      Serial.print( "command mode" );
      break;
    default: 
      Serial.print( "diagnostic" ); 
      break; 
  }
  
    // Report temperature and humidity
  // Temperature reported in celcius first
  // Celcius is what comes out of the sensor
  // Converted and reported as farenheit second
  Serial.print( "," );
  Serial.print( humidity );
  Serial.print( "," );
  Serial.print( temperature );
  Serial.print( "," );
  Serial.println( ( temperature * 9 ) / 5 + 32 );

  // Give it a second before updating
  delay( 1000 );
  
  updateLCD(" Temp  Humidity",( temperature * 9 ) / 5 + 32,humidity);
  if(client.connect(server, 8001)>0) {
    logMsgToServer("temp" , "Temp Sensor", ( temperature * 9 ) / 5 + 32,"50");
  }
  else{
      Serial.println("not connected");
   }
     // we have read all we need from the server stop now
  if(!client.connected()) {
    client.stop();
  }
 
 
}


// Retrieve temperature and humidity values from HIH6130
// Takes humidity and temperature references
// Updates them inside the function
// Returns the sensor state as a value directly
byte getTemperatureHumidity( float &hdata, float &tdata )
{
 // Serial.println("Getting Temp and Humid");
  byte hhigh;
  byte hlow;
  byte state;
  byte thigh;
  byte tlow;

  // Let the sensor know we are coming
  Wire.beginTransmission( ADDRESS ); 
  Wire.endTransmission();
  delay( 100 );
 //Serial.print( "Read the data packets" );     
  // Read the data packets
  Wire.requestFrom( (int)ADDRESS, 4 );
  hhigh = Wire.read();
  hlow = Wire.read();
  thigh = Wire.read();
  tlow = Wire.read();
  Wire.endTransmission();
      
  // Slice of state bytes
  state = ( hhigh >> 6 ) & 0x03;
  
  // Clean up remaining humidity bytes
  hhigh = hhigh & 0x3f;
  
  // Shift humidity bytes into a value
  // Convert value to humidity per data sheet  
  hdata = ( ( (unsigned int)hhigh ) << 8 ) | hlow;
  hdata = hdata * 6.10e-3;
      
  // Shift temperature bytes into a value
  // Convert value to temperature per data sheet
  tdata = ( ( (unsigned int)thigh ) << 8 ) | tlow;
  tdata = tdata / 4;
  tdata = tdata * 1.007e-2 - 40.0;

  // Return the sensor state
  return state;
}

void updateLCD(String title,float temp,float humidity){
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print(title);
    // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  lcd.print("");
  lcd.print(temp);
  lcd.print((char)223);
  lcd.print("  ");
  lcd.print(humidity);
  lcd.print("%");
} 


void logMsgToServer(String code, String descr, float value,String lengthStr){
  char buffer2[14]; 
  String tempStr = dtostrf(value, 7,2,buffer2);
  //Serial.println(temp);
  client.println("POST /device/post/ HTTP/1.0");
  client.println("Content-Type: application/x-www-form-urlencoded; charset=utf-8");
  client.println("Host: 192.168.1.2:8001?refId=1&code=" + code + "&descr=" + descr + "&value=" + tempStr);
  client.println(lengthStr);
  client.println();
  //client.println(values);
  client.println();
}


