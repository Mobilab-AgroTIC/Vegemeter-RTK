#include "driver_button.h"
#include "common/config.h"
#include "common/logger.h"

#include "common/logger.h"


uint8_t get_button_label(){
  int state_button_1=digitalRead(BTN_LABEL_1_PIN);
  int state_button_2=digitalRead(BTN_LABEL_2_PIN);
  int state = 0;
  if (!state_button_1){
    state=1;
    LOG_INFO("[BUTTON] Label 1 active");
  }
  else if (!state_button_2){
    state =2;
    LOG_INFO("[BUTTON] Label 2 active");
  }
  else{
    state =0;
    LOG_INFO("[BUTTON] Label 0 active");
  }
  return state;
}

bool state_button_send_data(){
  if (!digitalRead(BTN_SEND_DATA_PIN)){
    LOG_INFO("[BUTTON] Send Data button active");
    return true;
  }
  else{
    LOG_INFO("[BUTTON] Send Data button inactive");
    return false;
  }
}