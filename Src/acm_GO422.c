#include <stdio.h>
#include "acm_GO422.h"
#include "main.h"
#include "GopherCAN.h"

// set up enables for each power supply (automatically set to on, turn off if power strays from usable range)

// the HAL_CAN struct. This example only works for a single CAN bus
CAN_HandleTypeDef* acm_hcan;

// Use this to define what module this board and other boards will be
#define THIS_MODULE_ID ACM_ID
#define OTHER_MODULE DLM_ID

// global variables
U8 last_button_state;
U8 drs_state = 0;
U8 active_aero_state = 0;
U8 valid_channel = 0; // 0 for no, 1 for yes
FLOAT_CAN_STRUCT data_channel_1;
FLOAT_CAN_STRUCT data_channel_2; // can add more channels if necessary

// DRS: Angle of attack, 0 to 50, that will be chosen from based on the data passed into the function
// Might want to change naming conventions since idk which angle refers to a high downforce positon
U8 high_rear_angle = 50;
U8 mid_high_rear_angle = 33;
U8 mid_low_rear_angle = 17;
U8 low_rear_angle = 0; // high downforce or low downforce position?

// Active Aero: Angle of attack, 0 to 30, that will be chosen from based on the data passed into the function
U8 high_front_angle = 30;
U8 mid_high_front_angle = 20;
U8 mid_low_front_angle = 10;
U8 low_front_angle = 0; // high downforce or low downforce?


// initialize GopherCAN
// What needs to happen on startup in order to run GopherCAN
void init(CAN_HandleTypeDef* hcan_ptr)
{
	acm_hcan = hcan_ptr;

	// initialize CAN
	// NOTE: CAN will also need to be added in CubeMX and code must be generated
	// Check the STM_CAN repo for the file "F0xx CAN Config Settings.pptx" for the correct settings
	if (init_can(acm_hcan, THIS_MODULE_ID, BXTYPE_MASTER))
	{
		// an error has occurred, stay here
		while (1);
	}

	// enable updating for data channels
	DATA_CHANNEL_1.update_enabled = TRUE;
	DATA_CHANNEL_2.update_enabled = TRUE;

	// enable all of the variables in GopherCAN for testing
	set_all_params_state(TRUE);

	// Set the function pointer of SET_LED_STATE. This means the function change_led_state()
	// will be run whenever this can command is sent to the module
}

// can_buffer_handling_loop
//  This loop will handle CAN RX software task and CAN TX hardware task. Should be
//  called every 1ms or as often as received messages should be handled
void can_buffer_handling_loop()
{
	// handle each RX message in the buffer
	if (service_can_rx_buffer())
	{
		// an error has occurred
	}

	// handle the transmission hardware for each CAN bus
	service_can_tx_hardware(acm_hcan);
}

int valid_data_channel(FLOAT_CAN_STRUCT data_channel)
{
	for(int i = 0; i < sizeof(DATA_CHANNELS)/sizeof(DATA_CHANNELS[0]); i++) //need to figure out why these macros aren't transfering properly
	{
		if(data_channel == DATA_CHANNELS[i])
		{
			return 1;
		}
	}
	return 0;
}

int valid_thresholds(FLOAT_CAN_STRUCT data_channel); // not sure how I can create a general function to check for if valid thresholds were set int the h

// main_loop: function to have module run constantly while turned on

// function to be called in the main() function to determine which state the rear wing should be
// at based on the data requested
int caculate_rear_wing_angle(FLOAT_CAN_STRUCT data_channel_1, FLOAT_CAN_STRUCT data_channel_2)
{
	// TO COMPLETE
	// pretty sure this is just gonna be a sad series of if statements unless someone has a more
	// elegant idea
	if(data_channel_1.data > DATA_CHANNEL_1_THRESHOLD_HIGH )
	{
		return high_rear_angle;
	}
	else if(data_channel_1.data > DATA_CHANNEL_1_THRESHOLD_MID)
	{
		return mid_high_rear_angle;
	}
	else if(data_channel_1.data > DATA_CHANNEL_1_THRESHOLD_LOW)
	{
		return mid_low_rear_angle;
	}
	else if(data_channel_1.data < DATA_CHANNEL_1_THRESHOLD_LOW)
	{
		return low_rear_angle;
	}

	// currently do not know how I want to implement more than one data channel
}

// function to be called in the main() function to determine which state the front wing should be
// at based on the data requested
int caculate_front_wing_angle(FLOAT_CAN_STRUCT data_channel_1, FLOAT_CAN_STRUCT data_channel_2)
{
	//TO COMPLETE
	if(data_channel_1.data > DATA_CHANNEL_1_THRESHOLD_HIGH)
	{
		return high_rear_angle;
	}
	else if(data_channel_1.data > DATA_CHANNEL_1_THRESHOLD_MID)
	{
		return mid_high_rear_angle;
	}
	else if(data_channel_1.data > DATA_CHANNEL_1_THRESHOLD_LOW)
	{
		return mid_low_rear_angle;
	}
	else if(data_channel_1.data < DATA_CHANNEL_1_THRESHOLD_LOW)
	{
		return low_rear_angle;
	}
}

// manual control of DRS, switch from high to low position when driver presses button
void on_button_press(void)
{
	// TO COMPLETE
}
// parameter request function for channel 1
void parameter_request_channel_1(void)
{
	if (HAL_GetTick() - DATA_CHANNEL_1.last_rx >= UPDATE_TIME) // update every 50Hz
	{
		// don't send another data request if the request is already pending
		// A timeout may also be worth including just in case something goes wrong
		if (DATA_CHANNEL_1.pending_response == FALSE)
		{
			request_parameter(PRIO_HIGH, OTHER_MODULE, DATA_CHANNEL_1.param_id);
		}
		data_channel_1 = DATA_CHANNEL_1.data;
	}
}

// function to convert angle to PWM

void main_loop()
{
// will need to figure out something for when we can use multiple data channels at once
	valid_channel = valid_data_channel(DATA_CHANNEL_1);
	if(valid_channel == 1)
	{
		drs_state = calculate_rear_wing_angle();
		active_aero_state = calculate_front_wing_angle();
	}
	else
	{
		drs_state = high_rear_angle; // assuming this is the high downforce position
		active_aero_state = high_front_angle; // assuming this is high downforce position
	}
}
