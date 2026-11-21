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
    LOG_DEBUG("[BUTTON] Label 1 actif");
  }
  else if (!state_button_2){
    state =2;
    LOG_DEBUG("[BUTTON] Label 2 actif");
  }
  else{
    state =0;
    LOG_DEBUG("[BUTTON] Aucun label actif");
  }
  return state;
}

bool state_button_send_data(){
  // Appelée pour chaque message MQTT : on ne logge (en INFO) que les changements d'état
  static int last_state = -1;
  bool active = !digitalRead(BTN_SEND_DATA_PIN);
  if ((int)active != last_state) {
    LOG_INFO("[BUTTON] Envoi des données %s", active ? "ACTIVÉ" : "DÉSACTIVÉ (messages ignorés)");
    last_state = active;
  }
  return active;
}