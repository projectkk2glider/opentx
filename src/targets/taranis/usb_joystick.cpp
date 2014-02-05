
#include "../../opentx.h"
#include "usb_joystick.h"


/*
  Prepare and send new USB data packet

  The format of HID_Buffer is defined by
  USB endpoint description can be found in 
  file usb_hid_joystick.c, variable HID_JOYSTICK_ReportDesc
*/
void usb_joystick_update(void)
{
  static uint8_t HID_Buffer[HID_IN_PACKET];
  
  pauseMixerCalculations();

  //buttons
  HID_Buffer[0] = 0; //buttons
  for (int i = 0; i < 8; ++i) {
    if ( channelOutputs[i+8] > 0 ) {
      HID_Buffer[0] |= (1 << i);
    } 
  }

  //analog values
  HID_Buffer[1] = channelOutputs[0] / 10;
  HID_Buffer[2] = channelOutputs[1] / 10;
  HID_Buffer[3] = channelOutputs[2] / 10;
  HID_Buffer[4] = channelOutputs[3] / 10;

  resumeMixerCalculations();
  USBD_HID_SendReport (&USB_OTG_dev, HID_Buffer, HID_IN_PACKET );
}
