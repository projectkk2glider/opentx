
#include "../../opentx.h"


/**
* @brief  USBD_HID_GetPos
* @param  None
* @retval Pointer to report
*/
uint8_t *USBD_HID_GetPos (void)
{
//  int8_t  x = 0 , y = 0 ;
  static uint8_t HID_Buffer [4];
  
  
  HID_Buffer[0] = 0;
  HID_Buffer[1] = channelOutputs[0] / 10;   //x axis (positive is right mouse movement)
  HID_Buffer[2] = channelOutputs[1] / 10;   //y axis (positive is down mouse movement)
  HID_Buffer[3] = 0;
  
  return HID_Buffer;
}


