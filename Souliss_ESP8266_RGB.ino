/**************************************************************************
    Souliss - Power Socket Porting for Expressif ESP8266

	It use static IP Addressing

    Load this code on ESP8266 board using the porting of the Arduino core
    for this platform.
        
***************************************************************************/

// Configure the framework
#include "bconf/MCU_ESP8266.h"              // Load the code directly on the ESP8266

// **** Define the WiFi name and password ****
#include "C:\Users\Administrator\Documents\Privati\ArduinoWiFiInclude\wifi.h"
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
			Souliss_Logic_T16bis(memory_map, LEDCONTROL, &data_changed);
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
        FAST_50ms() {   // We process the logic and relevant input and output every 50 milliseconds
        } 
        
		FAST_90ms() { 
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
			Timer_LED_Strip(LEDCONTROL);
			Timer_DimmableLight(LEDWHITE1);
			Timer_DimmableLight(LEDWHITE2);
		}
	}
} 

U8 Souliss_Logic_T16bis(U8 *memory_map, U8 slot, U8 *trigger)
{
	U8 i_trigger = 0;														// Internal trigger

																			// Look for input value, update output. If the output is not set, trig a data
																			// change, otherwise just reset the input

	if (memory_map[MaCaco_IN_s + slot] == Souliss_T1n_RstCmd)
		return 0;

	// Set a new color
	if (memory_map[MaCaco_IN_s + slot] == Souliss_T1n_Set)
	{
		// Set the new color
		for (U8 i = 1; i<4; i++)
		{
			memory_map[MaCaco_OUT_s + slot + i] = memory_map[MaCaco_IN_s + slot + i];
			memory_map[MaCaco_AUXIN_s + slot + i] = memory_map[MaCaco_IN_s + slot + i];
			memory_map[MaCaco_IN_s + slot + i] = Souliss_T1n_RstCmd;
		}

		// If the color is set as white
		if (((memory_map[MaCaco_OUT_s + slot + 1] >= 0xF0) &&
			(memory_map[MaCaco_OUT_s + slot + 2] >= 0xF0) &&
			(memory_map[MaCaco_OUT_s + slot + 3] >= 0xF0)))
		{
			// Set middle brightness
			//memory_map[MaCaco_AUXIN_s + slot] = BRIGHT_DEFAULT;

			//LYTWhite(memory_map[MaCaco_AUXIN_s + slot]);
		}
		else // Set the color
		{
			// Set the brightness value as the high between R, G and B
			memory_map[MaCaco_AUXIN_s + slot] = memory_map[MaCaco_AUXIN_s + slot + 1];
			if (memory_map[MaCaco_AUXIN_s + slot + 2] > memory_map[MaCaco_AUXIN_s + slot])
				memory_map[MaCaco_AUXIN_s + slot] = memory_map[MaCaco_AUXIN_s + slot + 2];
			if (memory_map[MaCaco_AUXIN_s + slot + 3] > memory_map[MaCaco_AUXIN_s + slot])
				memory_map[MaCaco_AUXIN_s + slot] = memory_map[MaCaco_AUXIN_s + slot + 3];

			// Set the color
			//LYTColor(memory_map[MaCaco_OUT_s + slot + 1],
				//memory_map[MaCaco_OUT_s + slot + 2],
				//memory_map[MaCaco_OUT_s + slot + 3]);
		}

		memory_map[MaCaco_OUT_s + slot] = Souliss_T1n_OnCoil;			// Switch on the output
		memory_map[MaCaco_IN_s + slot] = Souliss_T1n_RstCmd;			// Reset			
	}
	else if (memory_map[MaCaco_IN_s + slot] == Souliss_T1n_ToggleCmd)		// Toggle Command
	{
		// Toggle the actual status of the light
		if (memory_map[MaCaco_OUT_s + slot] == Souliss_T1n_OffCoil)
			memory_map[MaCaco_IN_s + slot] = Souliss_T1n_OnCmd;
		else if (memory_map[MaCaco_OUT_s + slot] == Souliss_T1n_OnCoil)
			memory_map[MaCaco_IN_s + slot] = Souliss_T1n_OffCmd;
		else
			memory_map[MaCaco_IN_s + slot] = Souliss_T1n_RstCmd;
	}
	else if (memory_map[MaCaco_IN_s + slot] == Souliss_T1n_OffCmd)		// Off Command
	{
		memory_map[MaCaco_OUT_s + slot] = Souliss_T1n_OffCoil;		// Switch off the light state

		i_trigger = Souliss_TRIGGED;								// Trig the change

																	// Save the actual color
		for (U8 i = 1; i<4; i++)
			memory_map[MaCaco_OUT_s + slot + i] = 0;

		// Send the off command
		//LYTOff();

		// Reset the input
		memory_map[MaCaco_IN_s + slot] = Souliss_T1n_RstCmd;
	}
	else if (memory_map[MaCaco_IN_s + slot] == Souliss_T1n_OnCmd)
	{
		i_trigger = Souliss_TRIGGED;									// Trig the change

		memory_map[MaCaco_OUT_s + slot] = Souliss_T1n_OnCoil;			// Switch on the output

																		// If there were no color set, use a light white
		if ((memory_map[MaCaco_AUXIN_s + slot + 1] == 0) && (memory_map[MaCaco_AUXIN_s + slot + 2] == 0) && (memory_map[MaCaco_AUXIN_s + slot + 3] == 0))
		{
			for (U8 i = 1; i<4; i++)
				memory_map[MaCaco_AUXIN_s + slot + i] = 0xFF;				// Set a light white

			//memory_map[MaCaco_AUXIN_s + slot] = BRIGHT_DEFAULT;				// Set default brightness
		}

		// If the color is set as white
		if (((memory_map[MaCaco_AUXIN_s + slot + 1] >= 0xF0) &&
			(memory_map[MaCaco_AUXIN_s + slot + 2] >= 0xF0) &&
			(memory_map[MaCaco_AUXIN_s + slot + 3] >= 0xF0)))
		{
			// Set the output
			for (U8 i = 1; i<4; i++)
				memory_map[MaCaco_OUT_s + slot + i] = memory_map[MaCaco_AUXIN_s + slot + i];

			//LYTWhite(memory_map[MaCaco_AUXIN_s + slot], FADEENABLE);
		}
		else // Set the color
		{
			// Get the base brightness
			uint8_t base_bright = memory_map[MaCaco_AUXIN_s + slot + 1];
			if (memory_map[MaCaco_AUXIN_s + slot + 2] > base_bright)
				base_bright = memory_map[MaCaco_AUXIN_s + slot + 2];
			if (memory_map[MaCaco_AUXIN_s + slot + 3] > base_bright)
				base_bright = memory_map[MaCaco_AUXIN_s + slot + 3];

			float r = memory_map[MaCaco_AUXIN_s + slot + 1] + (memory_map[MaCaco_AUXIN_s + slot] - base_bright);
			float g = memory_map[MaCaco_AUXIN_s + slot + 2] + (memory_map[MaCaco_AUXIN_s + slot] - base_bright);
			float b = memory_map[MaCaco_AUXIN_s + slot + 3] + (memory_map[MaCaco_AUXIN_s + slot] - base_bright);

			if (r > (0xF0 - 1))	memory_map[MaCaco_OUT_s + slot + 1] = (0xF0 - 1);
			else if (r < 0)					memory_map[MaCaco_OUT_s + slot + 1] = 0;
			else							memory_map[MaCaco_OUT_s + slot + 1] = r;

			if (g >(0xF0 - 1))	memory_map[MaCaco_OUT_s + slot + 2] = (0xF0 - 1);
			else if (g < 0)					memory_map[MaCaco_OUT_s + slot + 2] = 0;
			else							memory_map[MaCaco_OUT_s + slot + 2] = g;

			if (b >(0xF0 - 1))	memory_map[MaCaco_OUT_s + slot + 3] = (0xF0 - 1);
			else if (b < 0)					memory_map[MaCaco_OUT_s + slot + 3] = 0;
			else							memory_map[MaCaco_OUT_s + slot + 3] = b;

			//LYTColor(memory_map[MaCaco_OUT_s + slot + 1], memory_map[MaCaco_OUT_s + slot + 2], memory_map[MaCaco_OUT_s + slot + 3], FADEENABLE);

			for (U8 i = 1; i<4; i++)
				memory_map[MaCaco_AUXIN_s + slot + i] = memory_map[MaCaco_AUXIN_s + slot + i];
		}

		memory_map[MaCaco_IN_s + slot] = Souliss_T1n_RstCmd;			// Reset	
	}
	else if (memory_map[MaCaco_IN_s + slot] == Souliss_T1n_BrightUp)				// Increase the light value 
	{
		// Increase the brightness
		if (memory_map[MaCaco_AUXIN_s + slot] < (255 - Souliss_T1n_BrightValue))	memory_map[MaCaco_AUXIN_s + slot] = memory_map[MaCaco_AUXIN_s + slot] + Souliss_T1n_BrightValue;	// Increase the light value

																																										// If is white
		//if ((memory_map[MaCaco_AUXIN_s + slot + 1] >= 0xF0) && (memory_map[MaCaco_AUXIN_s + slot + 2] >= 0xF0) && (memory_map[MaCaco_AUXIN_s + slot + 3] >= 0xF0))
			//LYTWhite(memory_map[MaCaco_AUXIN_s + slot], FADEENABLE);
		//else	// Otherwise
		//{
			// Get the base brightness
			uint8_t base_bright = memory_map[MaCaco_AUXIN_s + slot + 1];
			if (memory_map[MaCaco_AUXIN_s + slot + 2] > base_bright)
				base_bright = memory_map[MaCaco_AUXIN_s + slot + 2];
			if (memory_map[MaCaco_AUXIN_s + slot + 3] > base_bright)
				base_bright = memory_map[MaCaco_AUXIN_s + slot + 3];

			float r = memory_map[MaCaco_AUXIN_s + slot + 1] + (memory_map[MaCaco_AUXIN_s + slot] - base_bright);
			float g = memory_map[MaCaco_AUXIN_s + slot + 2] + (memory_map[MaCaco_AUXIN_s + slot] - base_bright);
			float b = memory_map[MaCaco_AUXIN_s + slot + 3] + (memory_map[MaCaco_AUXIN_s + slot] - base_bright);

			if (r > (0xF0 - 1))	memory_map[MaCaco_OUT_s + slot + 1] = (0xF0 - 1);
			else if (r < 0)					memory_map[MaCaco_OUT_s + slot + 1] = 0;
			else							memory_map[MaCaco_OUT_s + slot + 1] = r;

			if (g >(0xF0 - 1))	memory_map[MaCaco_OUT_s + slot + 2] = (0xF0 - 1);
			else if (g < 0)					memory_map[MaCaco_OUT_s + slot + 2] = 0;
			else							memory_map[MaCaco_OUT_s + slot + 2] = g;

			if (b >(0xF0 - 1))	memory_map[MaCaco_OUT_s + slot + 3] = (0xF0 - 1);
			else if (b < 0)					memory_map[MaCaco_OUT_s + slot + 3] = 0;
			else							memory_map[MaCaco_OUT_s + slot + 3] = b;

			//LYTColor(memory_map[MaCaco_OUT_s + slot + 1], memory_map[MaCaco_OUT_s + slot + 2], memory_map[MaCaco_OUT_s + slot + 3], FADEENABLE);
		//}

			for (U8 i = 1; i<4; i++)
				memory_map[MaCaco_AUXIN_s + slot + i] = memory_map[MaCaco_AUXIN_s + slot + i];

		memory_map[MaCaco_IN_s + slot] = Souliss_T1n_RstCmd;						// Reset
	}
	else if (memory_map[MaCaco_IN_s + slot] == Souliss_T1n_BrightDown)				// Decrease the light value
	{
		// Decrease the brightness
		if (memory_map[MaCaco_AUXIN_s + slot] > (Souliss_T1n_BrightValue))	memory_map[MaCaco_AUXIN_s + slot] = memory_map[MaCaco_AUXIN_s + slot] - Souliss_T1n_BrightValue;		// Decrease the light value

																																											// If is white		
		//if ((memory_map[MaCaco_AUXIN_s + slot + 1] >= 0xF0) && (memory_map[MaCaco_AUXIN_s + slot + 2] >= 0xF0) && (memory_map[MaCaco_AUXIN_s + slot + 3] >= 0xF0))
			//LYTWhite(memory_map[MaCaco_AUXIN_s + slot], FADEENABLE);
		//else	// Otherwise
		//{
			// Get the base brightness
			uint8_t base_bright = memory_map[MaCaco_AUXIN_s + slot + 1];
			if (memory_map[MaCaco_AUXIN_s + slot + 2] > base_bright)
				base_bright = memory_map[MaCaco_AUXIN_s + slot + 2];
			if (memory_map[MaCaco_AUXIN_s + slot + 3] > base_bright)
				base_bright = memory_map[MaCaco_AUXIN_s + slot + 3];

			float r = memory_map[MaCaco_AUXIN_s + slot + 1] + (memory_map[MaCaco_AUXIN_s + slot] - base_bright);
			float g = memory_map[MaCaco_AUXIN_s + slot + 2] + (memory_map[MaCaco_AUXIN_s + slot] - base_bright);
			float b = memory_map[MaCaco_AUXIN_s + slot + 3] + (memory_map[MaCaco_AUXIN_s + slot] - base_bright);

			if (r > (0xF0 - 1))	memory_map[MaCaco_OUT_s + slot + 1] = (0xF0 - 1);
			else if (r < 0)					memory_map[MaCaco_OUT_s + slot + 1] = 0;
			else							memory_map[MaCaco_OUT_s + slot + 1] = r;

			if (g >(0xF0 - 1))	memory_map[MaCaco_OUT_s + slot + 2] = (0xF0 - 1);
			else if (g < 0)					memory_map[MaCaco_OUT_s + slot + 2] = 0;
			else							memory_map[MaCaco_OUT_s + slot + 2] = g;

			if (b >(0xF0 - 1))	memory_map[MaCaco_OUT_s + slot + 3] = (0xF0 - 1);
			else if (b < 0)					memory_map[MaCaco_OUT_s + slot + 3] = 0;
			else							memory_map[MaCaco_OUT_s + slot + 3] = b;

			//LYTColor(memory_map[MaCaco_OUT_s + slot + 1], memory_map[MaCaco_OUT_s + slot + 2], memory_map[MaCaco_OUT_s + slot + 3], FADEENABLE);
		//}

			for (U8 i = 1; i<4; i++)
				memory_map[MaCaco_AUXIN_s + slot + i] = memory_map[MaCaco_AUXIN_s + slot + i];

		memory_map[MaCaco_IN_s + slot] = Souliss_T1n_RstCmd;						// Reset
	}

	// Update the trigger
	if (i_trigger)
		*trigger = i_trigger;

	return i_trigger;

}