#include <stdio.h>
#include "acm_GO422.h"
#include "GopherCAN.h"

// set up enables for each power supply (automatically set to on, turn off if power strays from usable range)

// the HAL_CAN struct. This example only works for a single CAN bus
CAN_HandleTypeDef* acm_hcan;

// Use this to define what module this board and other boards will be
#define THIS_MODULE_ID ACM_ID
#define OTHER_MODULE DLM_ID

// global variables
// need to make a parameter for every possible data channel
FLOAT_CAN_STRUCT CHANNEL_1, CHANNEL_2, CHANNEL_3, CHANNEL_4, CHANNEL_5;
FLOAT_CAN_STRUCT* data_channels[NUM_CHANNELS] =
{
	    &brake_pressure,
	    &steering_anglels,
	    &yaw_rate,
	    &throttle_position,
	    &acceleration,
};

U8 last_button_state;
U8 drs_state = 0;
U8 active_aero_state = 0;
U8 valid_channel = 0; // 0 for no, 1 for yes
FLOAT_CAN_STRUCT data_channel_1;
FLOAT_CAN_STRUCT data_channel_2; // can add more channels if necessary
FLOAT_CAN_STRUCT user_choice;

// DRS: Angle of attack, 0 to 50, that will be chosen from based on the data passed into the function
// Might want to change naming conventions since idk which angle refers to a high downforce positon
U8 high_rear_angle = 50;
U8 mid_high_rear_angle = 33;
U8 mid_low_rear_angle = 17;
U8 low_rear_angle = 0; // high downforce or low downforce position?
U8 err_drs = 50; //assuming 50 is high downforce position

// Active Aero: Angle of attack, 0 to 30, that will be chosen from based on the data passed into the function
U8 high_front_angle = 30;
U8 mid_high_front_angle = 20;
U8 mid_low_front_angle = 10;
U8 low_front_angle = 0; // high downforce or low downforce?
U8 err_active_aero = 30; //assuming 30 is high downforce angle


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
	int i;
	for(i = 0; i < NUM_CHANNELS; i++) //need to figure out why these macros aren't transfering properly
	{
		if(data_channel.data == data_channels[i]->data)
		{
			return 1;
		}
	}
	return 0;
}

// main_loop: function to have module run constantly while turned on

// function to be called in the main() function to determine which state the rear wing should be
// at based on the data requested
int calculate_rear_wing_angle(FLOAT_CAN_STRUCT data_channel_1)
{
	// TO COMPLETE
	// pretty sure this is just gonna be a sad series of if statements unless someone has a more
	// elegant idea
	if(data_channel_1.data > DATA_CHANNEL_1_MAX || data_channel_1.data < DATA_CHANNEL_1_MIN)
	{
		return err_drs;
	}
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
	else
	{
		return low_rear_angle;
	}

	// currently do not know how I want to implement more than one data channel
}

// function to be called in the main() function to determine which state the front wing should be
// at based on the data requested
int calculate_front_wing_angle(FLOAT_CAN_STRUCT data_channel_1)
{
	if(data_channel_1.data > DATA_CHANNEL_1_MAX || data_channel_1.data < DATA_CHANNEL_1_MIN)
	{
		return err_drs;
	}
	else if(data_channel_1.data > DATA_CHANNEL_1_THRESHOLD_HIGH)
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
	else
	{
		return low_rear_angle;
	}
}

// manual control of DRS, switch from high to low position when driver presses button
void on_button_press(void); // need to add a pin for input capture, will set the state to high downforce
// parameter request function for channel 1
void parameter_request_channel_1(void)
{
	if(HAL_GetTick() - DATA_CHANNEL_1.last_rx >= UPDATE_TIME) // update every 50Hz
	{
		// don't send another data request if the request is already pending
		// A timeout may also be worth including just in case something goes wrong
		if(DATA_CHANNEL_1.pending_response == FALSE)
		{
			request_parameter(PRIO_HIGH, OTHER_MODULE, DATA_CHANNEL_1.param_id);
		}
		data_channel_1.data = DATA_CHANNEL_1.data;
	}
}

// function to convert angle to PWM
void PWM_generation(int drs_state, int active_aero_state)
{
	if(active_aero_state == high_front_angle)
	{
		//front wing left (stall at 1500)
		TIM1->CCR1 = 1833;
		//front wing right (stall at 1500)
		TIM1->CCR2 = 1833;
	}
	if(active_aero_state == mid_high_front_angle)
	{
		TIM1->CCR1 = 1722; //left
		TIM1->CCR2 = 1722; //right
	}
	if(active_aero_state == mid_low_front_angle)
	{
		TIM1->CCR1 = 1611; //left
		TIM1->CCR2 = 1611; //right
	}
	if(active_aero_state == low_front_angle)
	{
		TIM1->CCR1 = 1500; //left
		TIM1->CCR2 = 1500; //right
	}
	if(drs_state == high_rear_angle)
	{
		TIM1->CCR3 = 2067; //drs
	}
	if(drs_state == mid_high_rear_angle)
	{
		TIM1->CCR3 = 1878; //drs
	}
	if(drs_state == mid_low_rear_angle)
	{
		TIM1->CCR3 = 1689; //drs
	}
	if(drs_state == low_rear_angle)
	{
		TIM1->CCR3 = 1500; //drs
	}
}

