/*
 *  This sketch demonstrates how to scan WiFi networks. 
 *  The API is almost the same as with the WiFi Shield library, 
 *  the most obvious difference being the different file you need to include:
 *  Comes with Arduino ESP8266 build under examples..
 
 Modified by DanBicks added OLED display to ESP01 using cheap Ebay 0.96" display
 Lightweight Mike-Rankin OLED routines added in this project, Credits to Mike for this.
 
 Display indicates WIFI Scan operation, looks for specific access point,
 connects to this if found and displays network connection details on OLED.
 
 Simple guide for users on how OLED can be implemented in to your project.
 My contribution to this superb community.
 
 Keep up the fantastic work everyone :)
 
 I hope you enjoy this sketch.
 
 Dans
 
 */
 
 
#include "ESP8266WiFi.h"
#include <Wire.h>
#include "font.h"
#define OLED_address  0x3c  //OLED I2C bus address


char* ssid     = "Your SSID Here";
char* password = "Your Password";
String MyNetworkSSID = "Your SSID Here"; // SSID you want to connect to Same as SSID


bool Fl_MyNetwork = false; // Used to flag specific network has been found
bool Fl_NetworkUP = false; // Used to flag network connected and operational.

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

char buffer[20]; 

extern "C" {
#include "user_interface.h"
}


/*
------------------------------------------------------------------------------------
                     Setup routines all located here!
------------------------------------------------------------------------------------
*/

void setup() {
  
  Serial.begin(115200);
  delay(2000); // wait for uart to settle and print Espressif blurb..
  
  // print out all system information
  Serial.print("Heap: "); Serial.println(system_get_free_heap_size());
  Serial.print("Boot Vers: "); Serial.println(system_get_boot_version());
  Serial.print("CPU: "); Serial.println(system_get_cpu_freq());
 
  Serial.println();
  Serial.println("OLED network Acquire Application Started....");
  
   //Initialize I2C and OLED Display
   //Wire.pins(int sda, int scl), etc 

  Wire.pins(0, 2); //on ESP-01.  
  Wire.begin();
  StartUp_OLED(); // Init Oled and fire up!
  Serial.println("OLED Init...");
  
  clear_display();
  sendStrXY(" DANBICKS WIFI ",0,1); // 16 Character max per line with font set
  sendStrXY("   SCANNER     ",2,1); 
  sendStrXY("START-UP ....  ",4,1); 
  delay(4000);

  Serial.println("Setup done");
 
  

}


/*
------------------------------------------------------------------------------------
                         Main Loop all loops here!
------------------------------------------------------------------------------------
*/


void loop() 

{
  
  
  
if (!Fl_NetworkUP)
  {
    Serial.println("Starting Process Scanning...");
    Scan_Wifi_Networks();
    Draw_WAVES();
    delay(2000);
    
      if (Fl_MyNetwork)
       {
    
        // Yep we have our network lets try and connect to it..
        Serial.println("MyNetwork has been Found....");
        Serial.println("Attempting Connection to Network..");
        Do_Connect();
    
           if (Fl_NetworkUP)
           {
           // Yep we all good Connected display to OLED
           Serial.println("Connected OK");
           Draw_WIFI();
           delay(4000); 
           clear_display(); // Clear OLED 
           IPAddress ip = WiFi.localIP(); // // Convert IP Here 
           String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
           ipStr.toCharArray(buffer, 20);   
           sendStrXY("NETWORK DETAILS",0,1);  
           sendStrXY("NET: ",3,1);
           sendStrXY((ssid),3,6);
           sendStrXY((buffer),6,1); // Print IP to yellow part OLED
                    
           }
          else
           {
           // No Joy in Connecting, Check wifi settings Password etc
           Serial.println("Not Connected");
           clear_display(); // Clear OLED 
           sendStrXY("CHECK   NETWORK",0,1);
           sendStrXY("   DETAILS     ",2,1);
           sendStrXY("NO CONNECTION..",6,1); // YELLOW LINE DISPLAY
           delay(3000);
           }
    
       }
       else
       {
        // Nope my network not identified in Scan  
        Serial.println("Not Connected");
        clear_display(); // Clear OLED 
        sendStrXY("  MY  NETWORK  ",0,1);
        sendStrXY("  NOT FOUND IN ",2,1);
        sendStrXY("     SCAN      ",4,1);
        delay(3000); 
         
       }

   }
  // Wait a little before trying again
  delay(5000);
  
  
  
} // end void loop



/*
------------------------------------------------------------------------------------
                     WIFI Specific routines all located here!
------------------------------------------------------------------------------------
*/


void Scan_Wifi_Networks()

{
   int c = 0;
   char myStr[13];
   char mySSIDstr[20];

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  // Need to be in dicsonected mode to Run network Scan!
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
   // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  sprintf(myStr,"%d APs found\0",n);
  clear_display();
  sendStrXY(myStr,3,0); // display the number of APs found
    delay(2000);
 
  if (n == 0)
  {
    clear_display();
    sendStrXY("No networks found",3,0);
    delay(1000);
   }  
  else
  {

   clear_display(); // Clear OLED 
 
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
        sprintf(myStr,"%s",WiFi.SSID(i)); // copy in the SSID
        strncpy(mySSIDstr,myStr,12);      // then truncate it
        myStr[12] = 0;  
        sprintf(myStr,"%s %2d\0",myStr,WiFi.RSSI(i)); // append the RSSI
        sendStrXY(myStr,c,0);
        c++;
        if (c > 7){
         delay(2000);
         clear_display(); // Clear OLED 
         c = 0 ;
        }
        delay(2000);       
    } // end of for loop
  }  // end of else
} // end of ScanWifiNetworks function

/*
------------------------------------------------------------------------------------
                   Try to connect to the Found WIFI Network!
------------------------------------------------------------------------------------
*/

void Do_Connect()
{
  
   Serial.println();
   Serial.print("Connecting to ");
   Serial.println(ssid);
  
   WiFi.begin(ssid, password);
  
   for(int i=0;i<10;i++)
   
     {
        
       if (WiFi.status() != WL_CONNECTED)
         {
          Fl_NetworkUP = false;     
         }
         else
         {
         Serial.println("");
         Serial.println("WiFi connected");  
         Serial.println("IP address: ");
         Serial.println(WiFi.localIP());         
         Fl_NetworkUP = true; 
         return;  
         }  
         
       delay(500);
       Serial.print(".");
     }
 
}// end of sub

