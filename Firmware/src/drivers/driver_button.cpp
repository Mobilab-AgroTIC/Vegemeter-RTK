#include "driver_button.h"
#include "common/config.h"



uint8_t get_button_label(){
  int state_button_1=digitalRead(BTN_LABEL_1_PIN);
  int state_button_2=digitalRead(BTN_LABEL_2_PIN);
  int state = 0;
  if (!state_button_1){
    state=1;
  }
  else if (!state_button_2){
    state =2;
  }
  else{
    state =0;
  }
  return state;
}

bool state_button_send_data(){
  if (!digitalRead(BTN_SEND_DATA_PIN)){
    return true;
  }
  else{
    return false;
  }
}