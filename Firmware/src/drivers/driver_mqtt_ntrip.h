#include "common/messages.h"

void setup_mqtt(const char* mqtt_server, const char* mqtt_user, const char* mqtt_password, uint16_t mqtt_port);
void prepare_and_send_mqtt_message(const fused_message_t &msg);
void setup_ntrip(char* host,int port,char* mntpnt,char* user,char* psw);
void setup_wifi(const char* ssid, const char* password);
