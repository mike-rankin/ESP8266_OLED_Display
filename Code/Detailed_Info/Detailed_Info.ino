/*
  Works with 8266-Arduino I2C OLED display !!
  partly imitated from http://thisoldgeek.blogspot.ca/2015/01/esp8266-weather-display.html
  Json Library: https://github.com/bblanchon/ArduinoJson
  for API key see: http://www.wunderground.com/weather/api/d/docs?d=index&MR=1 
*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include "font.h"
#include <ArduinoJson.h>
#define OLED_address  0x3c                           // OLED I2C bus address
#define SSID "YOUR SSID"                          // insert your SSID
#define PASS "YOUR PASSWORD"                            // insert your password

 
#define YOUR_KEY "YOUR KEY"                  // your (free) licence_key 
             
#define LOCATIONID "B0N2T0"                 // Location of your city      

//#define HOST "23.222.152.140"                        // api.wunderground.com
#define HOST "api.wunderground.com"

const int buffer_size = 300;                         // length of json buffer
char* conds[]={
  "\"city\":",
  "\"local_time_rfc822\":",
  "\"weather\":",
  "\"temp_c\":",
  "\"relative_humidity\":",
  "\"wind_dir\":",
  "\"wind_kph\":",
  "\"pressure_mb\":"
};
int num_elements = 8;                                // number of conditions to be retrieved, count of elements in conds
unsigned long WMillis = 0;                           // temporary millis() register


void setup() 
{
  Serial.begin(115200);                              // baudrate of monitor
  Wire.begin(0,2);                                   // I2C for ESP-01: (SDA,SCL) 
  WiFi.begin(SSID,PASS);                             // your WiFi Network's SSID & Password
    while (WiFi.status() != WL_CONNECTED) {          // DO until connected
    delay(500);                                      // 
    Serial.print(".");                               // print a few dots
  }
  Serial.println("");                            
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
  init_OLED();
  reset_display();
  displayOff();
  clear_display();
  displayOn();
  sendStrXY("Weather Info ESP", 0, 0);               // 
  sendStrXY(LOCATIONID, 2, 0);
  sendStrXY("SSID :", 5, 0);  sendStrXY(SSID, 5, 7); // prints SSID on OLED
  char result[16];
  sprintf(result, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  sendStrXY(result, 7, 0);  
  delay(10000);                                      // time to read local_IP
  wunderground();  
}


void loop()
{
     // Only check weather every 5-15 minutes, so you don't go over quota on wunderground (for free api license)
     if (millis()-WMillis >=600000) {                // 300 seconds interval
      wunderground();                                // get new data
      WMillis=millis();                              // 
     }
     // enough time here to do other stuff, like a bar that is showing how long it takes to update.
}


void wunderground() 
{
  Serial.print("connecting to ");
  Serial.println(HOST);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  
  const int httpPort = 80;
  
  if (!client.connect(HOST, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
  String cmd = "GET /api/";  cmd += YOUR_KEY;                   // build request_string cmd
  cmd += "/conditions/q/";  cmd += LOCATIONID;  cmd +=".json";  //
  cmd += " HTTP/1.1\r\nHost: api.wunderground.com\r\n\r\n";     // 
  delay(500);
  client.print(cmd);                                            // connect to api.wunderground.com with request_string
  delay(500);
  unsigned int i = 0;                                           // timeout counter
  char json[buffer_size]="{";                                   // first character for json-string is begin-bracket 
  int n = 1;                                                    // character counter for json  
  
  for (int j=0;j<num_elements;j++){                             // do the loop for every element/condition
    boolean quote = false; int nn = false;                      // if quote=fals means no quotes so comma means break
    while (!client.find(conds[j])){}                            // find the part we are interested in.
  
    String Str1 = conds[j];                                     // Str1 gets the name of element/condition
  
    for (int l=0; l<(Str1.length());l++)                        // for as many character one by one
        {json[n] = Str1[l];                                     // fill the json string with the name
         n++;}                                                  // character count +1
    while (i<5000) {                                            // timer/counter
      if(client.available()) {                                  // if character found in receive-buffer
        char c = client.read();                                 // read that character
          // Serial.print(c);                                   // 
// ************************ construction of json string converting comma's inside quotes to dots ********************        
               if ((c=='"') && (quote==false))                  // there is a " and quote=false, so start of new element
                  {quote = true;nn=n;}                          // make quote=true and notice place in string
               if ((c==',')&&(quote==true)) {c='.';}            // if there is a comma inside quotes, comma becomes a dot.
               if ((c=='"') && (quote=true)&&(nn!=n))           // if there is a " and quote=true and on different position
                  {quote = false;}                              // quote=false meaning end of element between ""
               if((c==',')&&(quote==false)) break;              // if comma delimiter outside "" then end of this element
 // ****************************** end of construction ******************************************************
          json[n]=c;                                            // fill json string with this character
          n++;                                                  // character count + 1
          i=0;                                                  // timer/counter + 1
        }
        i++;                                                    // add 1 to timer/counter
      }                    // end while i<5000
     if (j==num_elements-1)                                     // if last element
        {json[n]='}';}                                          // add end bracket of json string
     else                                                       // else
        {json[n]=',';}                                          // add comma as element delimiter
     n++;                                                       // next place in json string
  }
  //Serial.println(json);                                       // debugging json string 
  parseJSON(json);                                              // extract the conditions
  WMillis=millis();
}


void parseJSON(char json[300])
{
 StaticJsonBuffer<buffer_size> jsonBuffer;                    // class
 JsonObject& root = jsonBuffer.parseObject(json);
 
 if (!root.success())
{
  Serial.println("fparseObject() failed");
  return;
}
 const char* city        = root["city"];
 const char* local_time  = root["local_time_rfc822"];
 const char* weather     = root["weather"];
 double temp_c           = root["temp_c"]; 
 const char* humidity    = root["relative_humidity"];
 const char* wind_dir    = root["wind_dir"];
 double wind_kph         = root["wind_kph"];
 const char* pressure_mb = root["pressure_mb"];
 clear_display();                                             // clear OLED 
 char oled[32];                                               // string for condition element
  
  // Local date & time
  Serial.print("Time: ");  //Serial.println(local_time);
  sprintf(oled, "%s ", local_time);
  oled[3] = ' ';                                              // dot after dayname becomes a space                   
  for (int i = 0; i<16; i++) {sendCharXY(oled[i], 0, i);}     // display date  :"day. dd mmm yyyy" 16 chars
  for (int i = 0; i<12; i++) {sendCharXY(oled[i+17], 1, i);}  // displays time : hh:mm:ss -nn (nn is GMT hours) 
  sendStrXY("GMT", 1, 13);                                    // displays GMT
  
  // Conditions: Sunny, Cloudy, Fog, Rain, etc. 
  Serial.print("Weather: ");
  Serial.println(weather);
  sprintf(oled, "%s ", weather);
  sendStrXY(oled, 2, 0);
  
  // Temperature  
  Serial.print("Temp: ");
  Serial.println(temp_c);  
   sendStrXY("Temp    :", 3, 0);  
   sprintf(oled, "%d ", temp_c);
   dtostrf(temp_c, 3, 1, oled);
   sendStrXY(oled, 3, 10);
   for(int i = 0; i<8; i++)     // print degree symbol
     {SendChar(pgm_read_byte(myDregree+i));}
   sendStrXY("C", 3, 15);
 
  // Humidity 
  Serial.print("Humidity: ");
  Serial.println(humidity);
   sendStrXY("Humidity: ", 4, 0);
   sprintf(oled, "%s ", humidity);
   sendStrXY(oled, 4, 10);

  // Wind Direction
  Serial.print("Wind Direction: ");
  Serial.println(wind_dir);
   sendStrXY("Wind Dir:", 5, 0);
   sprintf(oled, "%s ", wind_dir);
   sendStrXY(oled, 5, 10);

  // Wind Speed
  Serial.print("Wind Speed: ");
  Serial.println(wind_kph);
   sendStrXY("Wind Spd:", 6, 0);
   sprintf(oled, "%d ", wind_kph);
   dtostrf(wind_kph, 3, 1, oled);
   sendStrXY(oled, 6, 10);
   sendStrXY("Km", 6, 14);
 
  // Barometric Pressure
  Serial.print("Barometric Pressure: ");
  Serial.println(pressure_mb);
   sendStrXY("Barmeter:", 7, 0);
   sprintf(oled, "%s ", pressure_mb);
   sendStrXY(oled, 7, 10);
}



//==========================================================//
// Resets display depending on the actual mode.
static void reset_display(void)
{
  displayOff();
  clear_display();
  displayOn();
}

//==========================================================//
// Turns display on.
void displayOn(void)
{
  sendcommand(0xaf);        //display on
}

//==========================================================//
// Turns display off.
void displayOff(void)
{
  sendcommand(0xae);		//display off
}

//==========================================================//
// Clears the display by sendind 0 to all the screen map.
static void clear_display(void)
{
  unsigned char i,k;
  for(k=0;k<8;k++)
  {	
    setXY(k,0);    
    {
      for(i=0;i<128;i++)     //clear all COL
      {
        SendChar(0);         //clear all COL
        //delay(10);
      }
    }
  }
}

//==========================================================//
// Actually this sends a byte, not a char to draw in the display. 
// Display's chars uses 8 byte font the small ones and 96 bytes
// for the big number font.
static void SendChar(unsigned char data) 
{
  //if (interrupt && !doing_menu) return;   // Stop printing only if interrupt is call but not in button functions
  
  Wire.beginTransmission(OLED_address); // begin transmitting
  Wire.write(0x40);//data mode
  Wire.write(data);
  Wire.endTransmission();    // stop transmitting
}

//==========================================================//
// Prints a display char (not just a byte) in coordinates X Y,
// being multiples of 8. This means we have 16 COLS (0-15) 
// and 8 ROWS (0-7).
static void sendCharXY(unsigned char data, int X, int Y)
{
  setXY(X, Y);
  Wire.beginTransmission(OLED_address); // begin transmitting
  Wire.write(0x40);//data mode
  
  for(int i=0;i<8;i++)
    Wire.write(pgm_read_byte(myFont[data-0x20]+i));
    
  Wire.endTransmission();    // stop transmitting
}

//==========================================================//
// Used to send commands to the display.
static void sendcommand(unsigned char com)
{
  Wire.beginTransmission(OLED_address);     //begin transmitting
  Wire.write(0x80);                          //command mode
  Wire.write(com);
  Wire.endTransmission();                    // stop transmitting
}

//==========================================================//
// Set the cursor position in a 16 COL * 8 ROW map.
static void setXY(unsigned char row,unsigned char col)
{
  sendcommand(0xb0+row);                //set page address
  sendcommand(0x00+(8*col&0x0f));       //set low col address
  sendcommand(0x10+((8*col>>4)&0x0f));  //set high col address
}

//==========================================================//
// Prints a string regardless the cursor position.
static void sendStr(unsigned char *string)
{
  unsigned char i=0;
  while(*string)
  {
    for(i=0;i<8;i++)
    {
      SendChar(pgm_read_byte(myFont[*string-0x20]+i));
    }
    *string++;
  }
}

//==========================================================//
// Prints a string in coordinates X Y, being multiples of 8.
// This means we have 16 COLS (0-15) and 8 ROWS (0-7).
static void sendStrXY( char *string, int X, int Y)
{
  setXY(X,Y);
  unsigned char i=0;
  while(*string)
  {
    for(i=0;i<8;i++)
    {
      SendChar(pgm_read_byte(myFont[*string-0x20]+i));
    }
    *string++;
  }
}

//==========================================================//
// Inits oled and draws logo at startup
static void init_OLED(void)
{
  sendcommand(0xae);		//display off
  sendcommand(0xa6);            //Set Normal Display (default)
    // Adafruit Init sequence for 128x64 OLED module
    sendcommand(0xAE);             //DISPLAYOFF
    sendcommand(0xD5);            //SETDISPLAYCLOCKDIV
    sendcommand(0x80);            // the suggested ratio 0x80
    sendcommand(0xA8);            //SSD1306_SETMULTIPLEX
    sendcommand(0x3F);
    sendcommand(0xD3);            //SETDISPLAYOFFSET
    sendcommand(0x0);             //no offset
    sendcommand(0x40 | 0x0);      //SETSTARTLINE
    sendcommand(0x8D);            //CHARGEPUMP
    sendcommand(0x14);
    sendcommand(0x20);             //MEMORYMODE
    sendcommand(0x00);             //0x0 act like ks0108
    
    //sendcommand(0xA0 | 0x1);      //SEGREMAP   //Rotate screen 180 deg
    sendcommand(0xA0);
    
    //sendcommand(0xC8);            //COMSCANDEC  Rotate screen 180 Deg
    sendcommand(0xC0);
    
    sendcommand(0xDA);            //0xDA
    sendcommand(0x12);           //COMSCANDEC
    sendcommand(0x81);           //SETCONTRAS
    sendcommand(0xCF);           //
    sendcommand(0xd9);          //SETPRECHARGE 
    sendcommand(0xF1); 
    sendcommand(0xDB);        //SETVCOMDETECT                
    sendcommand(0x40);
    sendcommand(0xA4);        //DISPLAYALLON_RESUME        
    sendcommand(0xA6);        //NORMALDISPLAY             

  clear_display();
  sendcommand(0x2e);            // stop scroll
  //----------------------------REVERSE comments----------------------------//
    sendcommand(0xa0);		//seg re-map 0->127(default)
    sendcommand(0xa1);		//seg re-map 127->0
    sendcommand(0xc8);
    delay(1000);
  //----------------------------REVERSE comments----------------------------//
  // sendcommand(0xa7);  //Set Inverse Display  
  // sendcommand(0xae);		//display off
  sendcommand(0x20);            //Set Memory Addressing Mode
  sendcommand(0x00);            //Set Memory Addressing Mode ab Horizontal addressing mode
  //  sendcommand(0x02);         // Set Memory Addressing Mode ab Page addressing mode(RESET)  
  
   setXY(0,0);
  
  for(int i=0;i<128*8;i++)     // show 128* 64 Logo
  {
    SendChar(pgm_read_byte(logo+i));
  }
  sendcommand(0xaf);		//display on
  
  //sendStrXY("Miker",7,5);
  delay(5000); 
}
