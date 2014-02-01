
#include "../../opentx.h"
#include "usb_joystick.h"

void usb_joystick_update(void)
{
  //get current joystick values 
  static uint8_t HID_Buffer [4];
  
  HID_Buffer[0] = 0;
  HID_Buffer[1] = channelOutputs[0] / 10;   //x axis (positive is right mouse movement)
  HID_Buffer[2] = channelOutputs[1] / 10;   //y axis (positive is down mouse movement)
  HID_Buffer[3] = 0;
  
  //send new values
  if((HID_Buffer[1] != 0) ||(HID_Buffer[2] != 0))
  {
    USBD_HID_SendReport (&USB_OTG_dev, HID_Buffer, sizeof(HID_Buffer) );
  }     
}
