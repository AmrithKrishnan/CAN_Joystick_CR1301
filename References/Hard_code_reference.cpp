#include <Arduino.h>

#include "SimpleJ1939.hpp"
#include <mcp_can.h>

typedef struct joystick_status_t{   // Structure to hold all the key variables 
    //6 digital keys
    /*
    key1            key4                    KEY POSITIONS ON JOYSTICK
    key2            key5
    key3            key6    
    */
    bool key1;
    bool key2;
    bool key3;
    bool key4;
    bool key5;
    bool key6;

    uint8_t rot_steps; // for the encoder incremental values
    bool rot_dir;   // to indicate CW and CCW rotation
    // FOUR TILT POSITIONS OF THE ROTARY BUTTON
    bool rot_up;
    bool rot_down;
    bool rot_left;
    bool rot_right;

    bool rot_mid; // Rotary button press

}joystick_status_t;

typedef struct color_t { // To hold RGB values of a color

  uint8_t r;
  uint8_t g;
  uint8_t b;
}color_t;

typedef struct led_status_t{ // To hold the led status 

  uint8_t key[6];
  uint8_t corona[4];
}led_colors_t;

//****************************************************************
// function declarations
//****************************************************************
bool joystick_get_status(joystick_status_t *status);
bool joystick_update_color(void);
void reset_stuff(void);

//****************************************************************
// global variables
//****************************************************************
color_t color_wheel[12] = { // to hold RGB values of the color wheel, used in color change mode
  {.r = 0xFF, .g = 0xFF, .b = 0x00},
  {.r = 0xFF, .g = 0xBF, .b = 0x00},
  {.r = 0xFF, .g = 0x7F, .b = 0x00},
  {.r = 0xFF, .g = 0x3F, .b = 0x00},
  {.r = 0xFF, .g = 0x00, .b = 0x00},
  {.r = 0xBF, .g = 0x00, .b = 0x3F},
  {.r = 0x7F, .g = 0x00, .b = 0x7F},
  {.r = 0x3F, .g = 0x00, .b = 0xBF},
  {.r = 0x00, .g = 0x00, .b = 0xFF},
  {.r = 0x00, .g = 0x7F, .b = 0x7F},
  {.r = 0x00, .g = 0xFF, .b = 0x00},
  {.r = 0x7F, .g = 0xFF, .b = 0x00}
};

static unsigned constexpr MCP_CS = 5; //Chip Select Pin

MCP_CAN CAN0(MCP_CS);
SimpleJ1939 J1939(&CAN0);


led_status_t state_led_manual = {.key = {1, 1, 1, 1, 1, 1}, .corona = {0}};
led_status_t state_led_auto = {.key = {1, 1, 1, 1, 1, 1}, .corona = {1, 1, 1 ,1}};
led_status_t *state_led = &state_led_manual;

int8_t state_color_manual = 0;                  // range 0:11 to determine the current color
int8_t state_color_auto = 0;                    // range 0:11 to determine the current color
int8_t *state_color = &state_color_manual;       // range 0:11

bool mode_manual_auto = false;                  // false:manual, true:auto
bool mode_pos_color = false;                    // false:position, true:color selection
int8_t rotory_pos = 0;                          // range 0:23

uint8_t last_sector = 0;

uint16_t color_change_period = 500;


//****************************************************************
// start
//****************************************************************
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

void loop(){

  static unsigned long time_to_change_color = 3000;               // **************************** need of update by entering automode
  joystick_status_t current_joystick_stauts;
  static bool last_state_key1 = false;
  static bool last_state_rot_mid = false;


  if(mode_manual_auto){
    // auto mode
    Serial.print("A");
    
    if (joystick_get_status(&current_joystick_stauts)){

      if (current_joystick_stauts.key4){reset_stuff();}

      // change mode to manual
      if (current_joystick_stauts.key1 && !last_state_key1){
        mode_manual_auto = false;
        state_led = &state_led_manual;
        state_color = &state_color_manual;
      }
      last_state_key1 = current_joystick_stauts.key1;

      if(current_joystick_stauts.rot_steps){
          // select frequency
          if(current_joystick_stauts.rot_dir){
            if(color_change_period < 3000){color_change_period = color_change_period * 1.2;}
          }else{
            if(50 < color_change_period){color_change_period = color_change_period / 1.2;}
          }
        }
    }

    // change color every few .. 
    if(time_to_change_color < millis()){
      time_to_change_color += color_change_period;
      state_color_auto++;if(12 <= state_color_auto){state_color_auto = 0;}
      state_color = &state_color_auto;
      
      joystick_update_color();
    }
  }else{
    //manual mode
    Serial.print("M");

    if (joystick_get_status(&current_joystick_stauts)){

      if (current_joystick_stauts.key4){reset_stuff();}

      //change mode to auto
      if (current_joystick_stauts.key1 && !last_state_key1){
        mode_manual_auto = true;
        state_led = &state_led_auto;
        state_color = &state_color_auto;
      }
      last_state_key1 = current_joystick_stauts.key1;

      if (mode_pos_color){
        // color
        Serial.print("C");
        // change mode to pos
        if (current_joystick_stauts.rot_mid && !last_state_rot_mid){mode_pos_color = false;}
        last_state_rot_mid = current_joystick_stauts.rot_mid;

        if (current_joystick_stauts.rot_steps){
          // select color
          if(!current_joystick_stauts.rot_dir){
            state_color_manual++;if(12 <= state_color_manual){state_color_manual = 0;}
          }else{
            state_color_manual--;if(state_color_manual < 0){state_color_manual = 12;}
          }
        }

      }else{
        // pos

        // change mode to color
        if (current_joystick_stauts.rot_mid && !last_state_rot_mid){mode_pos_color = true;}
        last_state_rot_mid = current_joystick_stauts.rot_mid;

        if (current_joystick_stauts.rot_steps){
          // select sector
          if(!current_joystick_stauts.rot_dir){
            rotory_pos++;if(24 <= rotory_pos){rotory_pos = 0;}
          }else{
            rotory_pos--;if(rotory_pos < 0){rotory_pos = 24;}
          }

          
          uint8_t current_sector = 0;
          if( 0 <= rotory_pos && rotory_pos <  6){current_sector = 0;}
          if( 6 <= rotory_pos && rotory_pos < 12){current_sector = 1;}
          if(12 <= rotory_pos && rotory_pos < 18){current_sector = 2;}
          if(18 <= rotory_pos && rotory_pos < 24){current_sector = 3;}
          if(current_sector != last_sector){
            last_sector = current_sector;

            if(state_led_manual.corona[current_sector]){
              state_led_manual.corona[current_sector] = 0;
            }else{
              state_led_manual.corona[current_sector] = 1;
            }
          }

          
        }
      }
      
      joystick_update_color();
    }
  }
}

//****************************************************************
// function definitions
//****************************************************************
bool joystick_get_status(joystick_status_t *status){

  byte nPriority;
  byte nSrcAddr;
  byte nDestAddr;
  byte nData[8];
  int nDataLen;
  long lPGN;

  // Check for received J1939 messages
  if (J1939.Receive(&lPGN, &nPriority, &nSrcAddr, &nDestAddr, nData, &nDataLen)){return false;} // function returns 1 if no data
  if (nPriority != 0x4 || nSrcAddr != 0x87 || lPGN != 0xFF64){return false;}

  status->key1 = nData[1] & (1<<0); // bitwise AND operator to extract the bit values and store in key
  status->key2 = nData[1] & (1<<2);
  status->key3 = nData[1] & (1<<4);
  status->key4 = nData[1] & (1<<6);
  status->key5 = nData[2] & (1<<0);
  status->key6 = nData[2] & (1<<2);

  status->rot_steps = nData[3] & (0xF<<0);
  status->rot_dir   = nData[3] & (1<<5);
  status->rot_mid   = nData[3] & (1<<6);
  status->rot_left  = nData[4] & (1<<5);
  status->rot_right = nData[4] & (1<<5);
  status->rot_up    = nData[4] & (1<<5);
  status->rot_down  = nData[4] & (1<<5);

  return true;
}

bool joystick_update_color(void){

  byte payload_but_1_2[8] = {color_wheel[*state_color].r * state_led->key[0],
    color_wheel[*state_color].g * state_led->key[0],
    color_wheel[*state_color].b * state_led->key[0],
    color_wheel[*state_color].r * state_led->key[1],
    color_wheel[*state_color].g * state_led->key[1],
    color_wheel[*state_color].b * state_led->key[1],
    0xFF, 0xFF};

  byte payload_but_3_4[8] = {color_wheel[*state_color].r * state_led->key[2],
    color_wheel[*state_color].g * state_led->key[2],
    color_wheel[*state_color].b * state_led->key[2],
    color_wheel[*state_color].r * state_led->key[3],
    color_wheel[*state_color].g * state_led->key[3],
    color_wheel[*state_color].b * state_led->key[3],
    0xFF, 0xFF};

  byte payload_but_5_6[8] = {color_wheel[*state_color].r * state_led->key[4],
    color_wheel[*state_color].g * state_led->key[4],
    color_wheel[*state_color].b * state_led->key[4],
    color_wheel[*state_color].r * state_led->key[5],
    color_wheel[*state_color].g * state_led->key[5],
    color_wheel[*state_color].b * state_led->key[5],
    0xFF, 0xFF};

  byte payload_corona_1_2[8] = {color_wheel[*state_color].r * state_led->corona[0],
    color_wheel[*state_color].g * state_led->corona[0],
    color_wheel[*state_color].b * state_led->corona[0],
    color_wheel[*state_color].r * state_led->corona[1],
    color_wheel[*state_color].g * state_led->corona[1],
    color_wheel[*state_color].b * state_led->corona[1],
    0xFF, 0xFF};
  
  byte payload_corona_3_4[8] = {color_wheel[*state_color].r * state_led->corona[2],
    color_wheel[*state_color].g * state_led->corona[2],
    color_wheel[*state_color].b * state_led->corona[2],
    color_wheel[*state_color].r * state_led->corona[3],
    color_wheel[*state_color].g * state_led->corona[3],
    color_wheel[*state_color].b * state_led->corona[3],
    0xFF, 0xFF};

  if (J1939.Transmit(0xFF75, 0x06, 0x87, 0x87, &payload_but_1_2[0], 8)){return false;}
  if (J1939.Transmit(0xFF76, 0x06, 0x87, 0x87, &payload_but_3_4[0], 8)){return false;}
  if (J1939.Transmit(0xFF77, 0x06, 0x87, 0x87, &payload_but_5_6[0], 8)){return false;}
  if (J1939.Transmit(0xFF78, 0x06, 0x87, 0x87, &payload_corona_1_2[0], 8)){return false;}
  if (J1939.Transmit(0xFF79, 0x06, 0x87, 0x87, &payload_corona_3_4[0], 8)){return false;}
};

void reset_stuff(void){

  mode_manual_auto = false;
  mode_pos_color = false;

  state_color = &state_color_manual;
  state_color_manual = 0;
  state_color_auto = 0;

  state_led = &state_led_manual;
  state_led_manual.corona[0] = 0;
  state_led_manual.corona[1] = 0;
  state_led_manual.corona[2] = 0;
  state_led_manual.corona[3] = 0;

  rotory_pos = 0;
  last_sector = 0;

  color_change_period = 500;

  joystick_update_color();
}
