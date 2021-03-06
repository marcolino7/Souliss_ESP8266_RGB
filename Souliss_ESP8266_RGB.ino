/**************************************************************************
    Souliss - Power Socket Porting for Expressif ESP8266

	It use static IP Addressing

    Load this code on ESP8266 board using the porting of the Arduino core
    for this platform.
        
***************************************************************************/

// Configure the framework
#include "bconf/MCU_ESP8266.h"              // Load the code directly on the ESP8266

// **** Define the WiFi name and password ****
#include "D:\__User\Administrator\Documents\Privati\ArduinoWiFiInclude\wifi.h"
//To avoide to share my wifi credentials on git, I included them in external file
//To setup your credentials remove my include, un-comment below 3 lines and fill with
//Yours wifi credentials
//#define WIFICONF_INSKETCH
//#define WiFi_SSID               "wifi_name"
//#define WiFi_Password           "wifi_password"    

// Include framework code and libraries
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include "Souliss.h"

#define VNET_DEBUG_INSKETCH
#define VNET_DEBUG  		1

// Define the network configuration according to your router settings
uint8_t ip_address[4]  = {192, 168, 1, 131};
uint8_t subnet_mask[4] = {255, 255, 255, 0};
uint8_t ip_gateway[4]  = {192, 168, 1, 1};

//Define Souliss Slot
#define LEDCONTROL        0
#define LEDBLUE            1
#define LEDGREEN          2
#define LEDRED           3
#define	LEDSTRIP1         4
#define LEDWHITE1         5
#define LEDSTRIP2         6
#define LEDWHITE2         7

// RGB FET
#define PIN_red    12
#define PIN_green  13
#define PIN_blue   15

// W FET
#define PIN_w1     14
#define PIN_w2     4


void setup()
{   
    Initialize();

	analogWriteFreq(500);
	analogWriteRange(255);

    // Connect to the WiFi network with static IP
	Souliss_SetIPAddress(ip_address, subnet_mask, ip_gateway);
    
	Set_LED_Strip(LEDCONTROL);                  // Set a logic to control a LED strip
	Set_DimmableLight(LEDSTRIP1);
	Set_DimmableLight(LEDSTRIP2);

	// Define inputs, outputs pins
	pinMode(12, OUTPUT);                 // Power the LED
	pinMode(13, OUTPUT);                 // Power the LED
	pinMode(15, OUTPUT);                 // Power the LED    
	pinMode(14, OUTPUT);                 // Power the LED
	pinMode(4, OUTPUT);                 // Power the LED 
	
	Serial.begin(115200);
    Serial.println("Node Init");
}

void loop()
{ 
    // Here we start to play
    EXECUTEFAST() {                     
        UPDATEFAST();   
		FAST_10ms() {

			// Execute the logic that handle the LED
			//Logic_LED_Strip(LEDCONTROL);
			Souliss_Logic_T16(memory_map, LEDCONTROL, &data_changed);
			Logic_DimmableLight(LEDWHITE1);
			Logic_DimmableLight(LEDWHITE2);

			// Use the output values to control the PWM
			analogWrite(PIN_red, mOutput(LEDRED));
			analogWrite(PIN_green, mOutput(LEDGREEN));
			analogWrite(PIN_blue, mOutput(LEDBLUE));
			analogWrite(PIN_w1, mOutput(LEDWHITE1));
			analogWrite(PIN_w2, mOutput(LEDWHITE2));

			// Just process communication as fast as the logics
			ProcessCommunication();
		}
        FAST_50ms() {
        } 
        
		FAST_90ms() { 
			Timer_LED_Strip(LEDCONTROL);
		}

		FAST_510ms() {
		}

        FAST_PeerComms();                                        
    }
	EXECUTESLOW()
	{
		UPDATESLOW();

		SLOW_10s() {

			// The timer handle timed-on states
			
			Timer_DimmableLight(LEDWHITE1);
			Timer_DimmableLight(LEDWHITE2);
		}
	}
} 