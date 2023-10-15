#include <Arduino.h>
#include "SimpleJ1939.hpp"
#include <mcp_can.h>

enum enum_state {manual = 0, color_change, reset, automatic};
enum enum_manual_state {initialize = 0, quad_A_CW, quad_B_CW, quad_C_CW, quad_D_CW, quad_D_CCW, quad_C_CCW, quad_B_CCW, quad_A_CCW};
//enum enum_auto_state {frequency_update};
//enum enum_reset_state {reset_LED_ON = 0, resest_LED_OFF};// AVOID SUBSTATES FOR THIS TOO??
//enum enum_color_change_state {color_change_LED_ON = 0, color_setup}; // MAYBE AVOID SUBSTATES FOR THIS ???

enum_state state = manual;
enum_manual_state manual_state = initialize;
uint8_t auto_state;
//enum_auto_state auto_state = frequency_update;
//enum_reset_state reset_state = reset_LED_ON;
//enum_color_change_state color_change_state = color_change_LED_ON;
bool entry = 1, auto_entry = 1, manual_entry = 1, color_change_entry = 1;
bool is_manual = 1;

unsigned long StartMillis, reset_state_millis = 0, auto_state_millis = 0;
int8_t rotary_position = 0, encoder_pos = 0;
uint16_t transition_time;
float factor = 1.0;

MCP_CAN CAN0(5);
SimpleJ1939 J1939(&CAN0);

struct keys
{
    //6 digital keys CHECK IF THIS IS CORRECT
    /*
    key1            key4                    KEY POSITIONS ON JOYSTICK
    key2            key5
    key3            key6    
    */
    bool digital_keys[6];
    bool up, down, left, right;
    bool rotary_press;
    bool rotary_direction;
    uint8_t encoder;
};

struct color
{
    uint8_t r, g, b;
};

class Joystick
{
public:
    keys joystick_keys;
    color current_color_selection;
    bool LED_status[10];
    //uint8_t encoder_pos;
    color color_wheel_manual[12] = 
    {
        {.r = 0xFF, .g = 0xFF, .b = 0x00},
        {.r = 0xFF, .g = 0xA5, .b = 0x00},
        {.r = 0xFF, .g = 0x66, .b = 0x00},
        {.r = 0xFF, .g = 0x3D, .b = 0x00},
        {.r = 0xFF, .g = 0x00, .b = 0x00},
        {.r = 0xA3, .g = 0x00, .b = 0x6D},
        {.r = 0x80, .g = 0x00, .b = 0x80},
        {.r = 0x4B, .g = 0x00, .b = 0x82},
        {.r = 0x00, .g = 0x00, .b = 0xFF},
        {.r = 0x00, .g = 0x7F, .b = 0xFF},
        {.r = 0x00, .g = 0xFF, .b = 0x00},
        {.r = 0x9A, .g = 0xCD, .b = 0x32}
    };

    color color_wheel_auto[6] = 
    {
        {.r = 0xFF, .g = 0x00, .b = 0x00},
        {.r = 0xFF, .g = 0xA5, .b = 0x00},
        {.r = 0xFF, .g = 0xFF, .b = 0x00},
        {.r = 0x00, .g = 0xFF, .b = 0x00},
        {.r = 0x00, .g = 0x00, .b = 0xFF},
        {.r = 0x80, .g = 0x00, .b = 0x80}
    };

    void CAN_send(color current_color, bool *LED, bool is_manu);
    void CAN_receive(keys *key_values);
} CR1301;

void Joystick::CAN_send(color current_color, bool *LED, bool is_manu)
{
    bool temp;
    byte payload_keys_1_2[8] = 
    {
        LED[0] * current_color.r,
        LED[0] * current_color.g,
        LED[0] * current_color.b,
        LED[1] * current_color.r,
        LED[1] * current_color.g,
        LED[1] * current_color.b,
        0xFF, 0xFF    
    };

    byte payload_keys_3_4[8] = 
    {
        LED[2] * current_color.r,
        LED[2] * current_color.g,
        LED[2] * current_color.b,
        LED[3] * current_color.r,
        LED[3] * current_color.g,
        LED[3] * current_color.b,
        0xFF, 0xFF    
    };

    byte payload_keys_5_6[8] = 
    {
        LED[4] * current_color.r,
        LED[4] * current_color.g,
        LED[4] * current_color.b,
        LED[5] * current_color.r,
        LED[5] * current_color.g,
        LED[5] * current_color.b,
        0xFF, 0xFF    
    };

    byte payload_rotary_1_2[8] = 
    {
        LED[6] * current_color.r,
        LED[6] * current_color.g,
        LED[6] * current_color.b,
        LED[7] * current_color.r,
        LED[7] * current_color.g,
        LED[7] * current_color.b,
        0xFF, 0xFF    
    };

    byte payload_rotary_3_4[8] = 
    {
        LED[8] * current_color.r,
        LED[8] * current_color.g,
        LED[8] * current_color.b,
        LED[9] * current_color.r,
        LED[9] * current_color.g,
        LED[9] * current_color.b,
        0xFF, 0xFF    
    };

    if (is_manu == 1 )
    {
    if(J1939.Transmit(0xFF79, 0x06, 0x87, 0xFF, &payload_rotary_3_4[0], 8)) temp = 1; //delay(30);
    if(J1939.Transmit(0xFF78, 0x06, 0x87, 0xFF, &payload_rotary_1_2[0], 8)) temp = 1; //delay(50);
    }
    else 
    {
    //for(int i=0; i<8; i++) Serial.println(payload_keys_1_2[i]); Serial.println();
    if(J1939.Transmit(0xFF79, 0x06, 0x87, 0xFF, &payload_rotary_3_4[0], 8)) temp = 1; //delay(30);
    if(J1939.Transmit(0xFF78, 0x06, 0x87, 0xFF, &payload_rotary_1_2[0], 8)) temp = 1; delay(50);
    if(J1939.Transmit(0xFF77, 0x06, 0x87, 0xFF, &payload_keys_5_6[0], 8)) temp = 1; //delay(30);
    if(J1939.Transmit(0xFF76, 0x06, 0x87, 0xFF, &payload_keys_3_4[0], 8)) temp = 1; delay(50);
    if(J1939.Transmit(0xFF75, 0x06, 0x87, 0xFF, &payload_keys_1_2[0], 8)) temp = 1; //delay(30);
    }

    //for(int i=0; i<8; i++) Serial.println(LED[i]); Serial.println();

    //Serial.println(LED[6]);
}

void Joystick::CAN_receive(keys *key_values)
{
    byte nPriority;
    byte nSrcAddr;
    byte nDestAddr;
    byte nData[8];
    int nDataLen;
    long lPGN;

    // Check for received J1939 messages
    if (J1939.Receive(&lPGN, &nPriority, &nSrcAddr, &nDestAddr, nData, &nDataLen) == 0)
    {
        if (nSrcAddr == 0x87 || lPGN == 0xFF64)
        {

            key_values->digital_keys[0] = nData[1] & (1<<0);
            key_values->digital_keys[1] = nData[1] & (1<<2);
            key_values->digital_keys[2] = nData[1] & (1<<4);
            key_values->digital_keys[3] = nData[1] & (1<<6);
            key_values->digital_keys[4] = nData[2] & (1<<0);
            key_values->digital_keys[5] = nData[2] & (1<<2);

            key_values->encoder = nData[3] & (0b00011111<<0);
            key_values->rotary_direction = nData[3] & (1<<5);
            key_values->rotary_press = nData[3] & (1<<6);
            key_values->left = nData[4] & (1<<0);
            key_values->right = nData[4] & (1<<2);
            key_values->up = nData[4] & (1<<4);
            key_values->down = nData[4] & (1<<6);
        }
    }
}

void setup()
{
    CR1301.current_color_selection = CR1301.color_wheel_manual[0]; // gving initial color value as yellow
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
    //TO DEFINE
}

void loop()
{
CR1301.CAN_receive(&CR1301.joystick_keys);
static keys prev_keys = CR1301.joystick_keys;

switch (state) // switch for top level states: manual, color change, reset, auto
{
    //------Manual Mode begins here---------------
    case manual:
        if(entry == 1) // Entry conditions for manual state
        {
            entry = 0;
            for (int i = 0; i < 10; i++) {CR1301.LED_status[i] = 0;} //All LEDs OFF
        }
        //-------During condition for manual state begins--------
        //---------- Sub states for Manual using nested switch case-----------
        switch (manual_state)
        {
            case initialize:
                if(manual_entry == 1) // entry condition for initialize sub state
                {
                    manual_entry = 0;
                    for (int i = 0; i < 10; i++) {CR1301.LED_status[i] = 0;} //All LEDs OFF
                }
                if (1) // exit condition to exit after just one iteration of initialize state
                {
                    manual_entry = 1;
                }
            break;
            //----------Clockwise rotation cases-----------------
            case quad_A_CW:
                if(manual_entry == 1) // entry condition
                {
                    manual_entry = 0;
                    CR1301.LED_status[6] = 1; //rotary LED1 ON
                }
                // exit condition to exit after just one iteration of initialize state
                if (1){manual_entry = 1;}
            break;
            case quad_B_CW:
                if(manual_entry == 1) // entry condition
                {
                    manual_entry = 0;
                    CR1301.LED_status[6] = 1; //rotary LED1 ON
                    CR1301.LED_status[7] = 1; //rotary LED2 ON
                }
                if (1){manual_entry = 1;}
            break;
            case quad_C_CW:
                if(manual_entry == 1) // entry condition
                {
                    manual_entry = 0;
                    CR1301.LED_status[6] = 1; //rotary LED1 ON
                    CR1301.LED_status[7] = 1; //rotary LED2 ON
                    CR1301.LED_status[8] = 1; //rotary LED3 ON
                }
                if (1){manual_entry = 1;}
            break;
            case quad_D_CW:
                if(manual_entry == 1) // entry condition
                {
                    manual_entry = 0;
                    CR1301.LED_status[6] = 1; //rotary LED1 ON
                    CR1301.LED_status[7] = 1; //rotary LED2 ON
                    CR1301.LED_status[8] = 1; //rotary LED3 ON
                    CR1301.LED_status[9] = 1; //rotary LED4 ON
                }
                if (1){manual_entry = 1;}
            break;
            //----------Clockwise rotation cases over------------
            //-------Counter Clockwise rotation cases-------------
            case quad_D_CCW:
                if(manual_entry == 1) // entry condition
                {
                    manual_entry = 0;
                    CR1301.LED_status[9] = 1; //rotary LED4 ON
                }
                if (1){manual_entry = 1;}
            break;
            case quad_C_CCW:
                if(manual_entry == 1) // entry condition
                {
                    manual_entry = 0;
                    CR1301.LED_status[9] = 1; //rotary LED4 ON
                    CR1301.LED_status[8] = 1; //rotary LED3 ON
                }
                if (1){manual_entry = 1;}
            break;
            case quad_B_CCW:
                if(manual_entry == 1) // entry condition
                {
                    manual_entry = 0;
                    CR1301.LED_status[9] = 1; //rotary LED4 ON
                    CR1301.LED_status[8] = 1; //rotary LED3 ON
                    CR1301.LED_status[7] = 1; //rotary LED2 ON
                }
                if (1){manual_entry = 1;}
            break;
            case quad_A_CCW:
                if(manual_entry == 1) // entry condition
                {
                    manual_entry = 0;
                    CR1301.LED_status[9] = 1; //rotary LED4 ON
                    CR1301.LED_status[8] = 1; //rotary LED3 ON
                    CR1301.LED_status[7] = 1; //rotary LED2 ON
                    CR1301.LED_status[6] = 1; //rotary LED1 ON
                }
                if (1){manual_entry = 1;}
            break;
            //------------Counter Clockwise rotation cases over-------------
        }
        //-------------Sub states of manual state are over----------
        if (CR1301.joystick_keys.encoder) // if encoder value is non zero, which means encoder has been rotated
        {
            // get value of the increments in encoder. store in a variable
            if(!CR1301.joystick_keys.rotary_direction) // Clockwise direction
            {   
                rotary_position++;
                //rotary_position += CR1301.joystick_keys.encoder;
                // OLD CODE SNIPPET rotary_position++; 
                if(rotary_position > 23) // check for full rotation
                {
                    rotary_position = 0;
                    manual_state = initialize;                
                } 
            }
            else // Counter Clockwise direction
            {
                //rotary_position -= CR1301.joystick_keys.encoder;
                // OLD CODE SNIPPET rotary_position--; 
                rotary_position++;
                if(rotary_position < -23) // check for full rotation
                {
                    rotary_position = 0;
                    manual_state = initialize; 
                }
            }
            // ---Set the manual state based on rotary position
            if(rotary_position > 0 && rotary_position < 6){manual_state = quad_A_CW;}
            if(rotary_position >= 6 && rotary_position < 12){manual_state = quad_B_CW;}
            if(rotary_position >= 12 && rotary_position < 18){manual_state = quad_C_CW;}
            if(rotary_position >= 18 && rotary_position < 24){manual_state = quad_D_CW;}
            if(rotary_position < 0 && rotary_position >= -5){manual_state = quad_D_CCW;}
            if(rotary_position <= -6 && rotary_position >= -11){manual_state = quad_C_CCW;}
            if(rotary_position <= -12 && rotary_position >= -17){manual_state = quad_B_CCW;}
            if(rotary_position <= -18 && rotary_position >= -23){manual_state = quad_A_CCW;}
        }
        CR1301.CAN_send(CR1301.current_color_selection, CR1301.LED_status, 1);
        //-------End of During conditions for manual state-------- 
        if (CR1301.joystick_keys.rotary_press == 1 && prev_keys.rotary_press == 0) // Exit condition for the manual state to color change state
        {
            entry = 1;
            state = color_change;
            manual_state = initialize; // To reset the manual state to initialize for next entry into manual state
            for (int i = 0; i < 10; i++) {CR1301.LED_status[i] = 0;} //All LEDs OFF
        }
        if (CR1301.joystick_keys.digital_keys[0] == 1 && prev_keys.digital_keys[0] == 0) // Exit condition for the manual state to reset state
        {
            entry = 1;
            state = reset;
            manual_state = initialize; // To reset the manual state to initialize for next entry into manual state
            for (int i = 0; i < 10; i++) {CR1301.LED_status[i] = 0;} //All LEDs OFF
        }
        if (CR1301.joystick_keys.digital_keys[5] == 1 && prev_keys.digital_keys[5] == 0) // Exit condition for the manual state to auto state
        {
            entry = 1;
            state = automatic;
            manual_state = initialize; // To reset the manual state to initialize for next entry into manual state
            for (int i = 0; i < 10; i++) {CR1301.LED_status[i] = 0;} //All LEDs OFF
        }
    break;
    //--------------End of Manual mode---------------
    //-------------Color change mode begins here--------------
    case color_change:
        if(entry == 1) // Entry conditions for color change state
        {
            entry = 0;
            rotary_position = 0;
            for (int i = 0; i < 10; i++) {CR1301.LED_status[i] = 1;} //All LEDs ON
        }
        //---------during condition for color change mode----------
        //------getting encoder values----------
        if(!CR1301.joystick_keys.rotary_direction) // Clockwise direction
        {
            //rotary_position++;
            rotary_position += CR1301.joystick_keys.encoder;
            if(rotary_position > 23) // check for full rotation
            {
                rotary_position = 0; //reset to value 0 after one full rotation               
            } 
        }
        else // Counter Clockwise direction
        {
            //rotary_position--;
            rotary_position -= CR1301.joystick_keys.encoder;
            if(rotary_position < -23) // check for full rotation
            {
                rotary_position = 0; //reset to value 0 after one full rotation 
            }
        }
        // ---Set the current color value based on rotary position
        for (int i = 0; i <= 22; i+= 2) // setting color wheel as per CW rotation
        {
            if(rotary_position >= i && rotary_position <= i+1)
            CR1301.current_color_selection = CR1301.color_wheel_manual[i/2];
        }
        for (int i = -1; i >= -23; i-= 2) // setting color wheel as per CCW rotation
        {
            if(rotary_position <= i && rotary_position >= i-1)
            CR1301.current_color_selection = CR1301.color_wheel_manual[-i/2];
        }
        //-------exit condition of color change mode. going back to manual state--------
        if (CR1301.joystick_keys.rotary_press == 1 && prev_keys.rotary_press == 0) // Exit condition
        {
            entry = 1;
            state = manual;
            for (int i = 0; i < 10; i++) {CR1301.LED_status[i] = 0;} //All LEDs OFF
        }
    break;
    //--------End of color change mode-------------
    //---------Reset mode begins here------------
    case reset:
        if(entry == 1) // entry conditions
        {
            reset_state_millis = millis();
            entry = 0;
            rotary_position = 0;
            CR1301.current_color_selection=CR1301.color_wheel_manual[0];
            for (int i = 0; i < 10; i++) {CR1301.LED_status[i] = 1;} //All LEDs ON
        }
        // Could add blinking in during BUT do not use delay() as transmission happens at end of loop()
        // Would increase the complexity. Check if time permits.
        if(millis() - reset_state_millis  > 4000 ) //exit condition to manual state after 4s delay
        {
            entry = 1;
            state = manual;
            for (int i = 0; i < 10; i++) {CR1301.LED_status[i] = 0;} //All LEDs OFF
        }
    break;
    //--------Reset mode ends here-----------
    //---------Automatic mode begins------------
    case automatic:
        if(entry == 1)
            {
                entry = 0;
                for (int i = 0; i < 10; i++) {CR1301.LED_status[i] = 1;} //All LEDs ON
            }
        //------update frequency in during phase of state-------
        if (CR1301.joystick_keys.encoder) // if encoder value is non zero
        {
        // get value of the increments in encoder. store in a variable
            if(!CR1301.joystick_keys.rotary_direction) // Clockwise direction
            {   
                encoder_pos += CR1301.joystick_keys.encoder;
                if(encoder_pos > 23){encoder_pos = 23;} 
            }
            else // Counter Clockwise direction
            {
                encoder_pos -= CR1301.joystick_keys.encoder;
                if(encoder_pos < -23){encoder_pos = -23;}
            }
            // ---Set the frequency based on rotary position
                if(encoder_pos >= 0 && encoder_pos < 6){transition_time = 750;}
                if(encoder_pos >= 6 && encoder_pos < 12){transition_time = 1000;}
                if(encoder_pos >= 12 && encoder_pos < 18){transition_time = 1250;}
                if(encoder_pos >= 18 && encoder_pos < 24){transition_time = 1500;}
                if(encoder_pos < 0 && encoder_pos >= -5){transition_time = 600;}
                if(encoder_pos <= -6 && encoder_pos >= -11){transition_time = 500;}
                if(encoder_pos <= -12 && encoder_pos >= -17){transition_time = 300;}
                if(encoder_pos <= -18 && encoder_pos >= -23){transition_time = 200;}
        }
        switch(auto_state) // To move through each color. delay given after each color
            {
                case 0:
                    delay(transition_time);
                    CR1301.current_color_selection = CR1301.color_wheel_auto[0];
                break;
                case 1:
                    delay(transition_time);
                    CR1301.current_color_selection = CR1301.color_wheel_auto[1];
                break;
                case 2:
                    delay(transition_time);
                    CR1301.current_color_selection = CR1301.color_wheel_auto[2];
                break;
                case 3:
                    delay(transition_time);
                    CR1301.current_color_selection = CR1301.color_wheel_auto[3];
                break;
                case 4:
                    delay(transition_time);
                    CR1301.current_color_selection = CR1301.color_wheel_auto[4];
                break;
                case 5:
                    delay(transition_time);
                    CR1301.current_color_selection = CR1301.color_wheel_auto[5];
                break;
            }
        if (CR1301.joystick_keys.digital_keys[5] == 1 && prev_keys.digital_keys[5] == 0) // Exit condition for the auto state to manual state
        {
            entry = 1;
            state = manual;
            CR1301.current_color_selection = CR1301.color_wheel_manual[0];
            encoder_pos = 0;
            transition_time = 0;
            for (int i = 0; i < 10; i++) {CR1301.LED_status[i] = 0;} //All LEDs OFF
        }
    break;
}
//CR1301.CAN_send(CR1301.current_color_selection, CR1301.LED_status);
prev_keys = CR1301.joystick_keys; // giving value at end of loop. so it will hold prev loop values in next loop
Serial.println(rotary_position);
}