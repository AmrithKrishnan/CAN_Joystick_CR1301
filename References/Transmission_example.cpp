#include "SimpleJ1939.hpp"
#include <mcp_can.h>
static unsigned constexpr MCP_CS = 5; //Chip Select Pin

MCP_CAN CAN0(MCP_CS);
SimpleJ1939 J1939(&CAN0);

struct color
{
    uint8_t r, g, b;
};

bool LED[10] = {1,1,1,1,1,1,1,1,1,1};
color current_color = {.r=0xFF, .g=0xFF, 0x00};

void setup()
{
  // Set the serial interface baud rate
  Serial.begin(115200);
  if(CAN0.begin(MCP_ANY, CAN_250KBPS, MCP_8MHZ) == CAN_OK)
  {
    CAN0.setMode(MCP_NORMAL); // Set operation mode to normal so the MCP2515 sends acks to received data.
  }
  else
  {
    Serial.print("CAN Init Failed. Check your SPI connections or CAN.begin intialization .\n\r");
    while(1);
  }
}

void loop()
{
  long PGN = 0xFF77;
  byte priority = 0x06;
  byte srcAddr = 0xFF;
  byte destAddr = 0x87;
  byte payload[8] = 
    {
        LED[4] * current_color.r,
        LED[4] * current_color.g,
        LED[4] * current_color.b,
        LED[5] * current_color.r,
        LED[5] * current_color.g,
        LED[5] * current_color.b,
        0xFF, 0xFF    
    };
  byte length = 8;
	
  if (J1939.Transmit(PGN, priority, srcAddr, destAddr, &payload[0], length) == 0)
  {
	  Serial.println("Message sent");
  }
  delay(1000);
}