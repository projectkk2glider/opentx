
#include "../../opentx.h"
#include "usb_joystick.h"



void usb_joystick_update(void)
{
  //get current joystick values 
  //static uint8_t HID_Buffer [4];
  static uint8_t HID_Buffer [8];
  
  
  pauseMixerCalculations();
  
  HID_Buffer[0] = 0; //butons
  HID_Buffer[1] = channelOutputs[0] / 10;   //x axis (positive is right mouse movement)
  HID_Buffer[2] = channelOutputs[1] / 10;   //y axis (positive is down mouse movement)
  HID_Buffer[3] = 0;
  HID_Buffer[4] = 0;

/*    
  for (int i = 0; i < 6; ++i)
  {
    HID_Buffer[i] = channelOutputs[i] / 10;
    //++p;
  }
  //buttons
  HID_Buffer[6] = 0;
  HID_Buffer[7] = 0;
 */ 
  resumeMixerCalculations();
  
  //while(1);
  
  //send new values
  //if((HID_Buffer[1] != 0) ||(HID_Buffer[2] != 0))
  {
    USBD_HID_SendReport (&USB_OTG_dev, HID_Buffer, 5 );
  }     
}
