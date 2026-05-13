#include"NTRIPClient.h"
#include "common/logger.h"
bool NTRIPClient::reqSrcTbl(char* host,int &port)
{
  if(!connect(host,port)){
    LOG_ERROR("[NTRIP] Cannot connect to %s", host);
    return false;
  }
  String p;
  p = String("GET /");
  p = p + String(" HTTP/1.0\r\n");
  p = p + String("Host: caster.centipede.fr:2101\r\n");
  p = p + String("User-Agent: NTRIP Client/buchedv1.01\r\n");
  p = p + String("Accept: text/html\r\n");
  p = p + String("User-Agent: Mozilla/5.0\r\n");
  p = p + String("Connection: keep-alive\r\n");
  p = p + String("\r\n");
  print(p);

  unsigned long timeout = millis();
  while (available() == 0) {
     if (millis() - timeout > 5000) {
        LOG_ERROR("[NTRIP] Client Timeout !");
        stop();
        return false;
     }
     vTaskDelay(pdMS_TO_TICKS(10));
  }
  char buffer[12];
  readLine(buffer,sizeof(buffer));
  if(strstr(buffer,"SOURCETABLE 200 OK")!=NULL)
  {
    LOG_INFO("[NTRIP] Source table received");
    LOG_DEBUG("[NTRIP] Buffer: %s", buffer);
    return false;
  }
  return true;
}

bool NTRIPClient::reqRaw(char* host,int &port,char* mntpnt,char* user,char* psw)
{
    if(!connect(host,port))return false;
    String p="GET /";
    String auth="";
    LOG_INFO("[NTRIP] Requesting raw data");
    
    p = p + mntpnt + String(" HTTP/1.0\r\n"
        "User-Agent: NTRIPClient for Arduino v1.0\r\n"
    );
    
    if (strlen(user)==0) {
        p = p + String(
            "Accept: */*\r\n"
            "Connection: close\r\n"
            );
    }
    else {
        auth = base64::encode(String(user) + String(":") + psw);
        #ifdef Debug
        Serial.println(String(user) + String(":") + psw);
        #endif

        p = p + String("Authorization: Basic ");
        p = p + auth;
        p = p + String("\r\n");
    }
    p = p + String("\r\n");
    print(p);
    #ifdef Debug
    Serial.println(p);
    #endif
    unsigned long timeout = millis();
    while (available() == 0) {
        if (millis() - timeout > 20000) {
            Serial.println("Client Timeout !");
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    char buffer[50];
    readLine(buffer,sizeof(buffer));
    if(strncmp((char*)buffer,"ICY 200 OK",10))
    {
      Serial.print((char*)buffer);
      return false;
    }
    return true;
}
bool NTRIPClient::reqRaw(char* host,int &port,char* mntpnt)
{
    return reqRaw(host,port,mntpnt,"","");
}
int NTRIPClient::readLine(char* _buffer,int size)
{
  int len = 0;
    unsigned long start = millis();
    while (len < size - 1) {
        if (available()) {
            _buffer[len] = read();
            if (_buffer[len++] == '\n') break;
        } else {
            if (millis() - start > 2000) break; // timeout safety
            vTaskDelay(pdMS_TO_TICKS(5)); // ← cède le CPU
        }
    }
    _buffer[len] = '\0';
    return len;
}
/* int NTRIPClient::readLine_old(char* _buffer,int size)
{
  int len = 0;
  while(available()) {
    _buffer[len] = read();
    len++;
    if(_buffer[len-1] == '\n' || len >= size) break;
  }
  _buffer[len]='\0';

  return len;
} */

void NTRIPClient::sendGGA(const char* ggaMessage, const char* host, int port, const char* user, const char* passwd, const char* mntpnt) {
    if (!connected()) {
        LOG_ERROR("[NTRIP] NTRIPClient not connected, reconnecting...");
        if (!connect(host, port)) {
            LOG_ERROR("[NTRIP] Reconnection failed");
            return;
        }
    }

    String p = String("GET /") + String(mntpnt) + String(" HTTP/1.0\r\n");
    p += String("Host: ") + String(host) + String(":") + String(port) + String("\r\n");
    p += String("User-Agent: NTRIP Client/buchedv1.01\r\n");
    p += String("Accept: */*\r\n");
    if (strlen(user) > 0) {
        String auth = base64::encode(String(user) + ":" + String(passwd));
        p += String("Authorization: Basic ") + auth + String("\r\n");
    }
    p += String("Connection: close\r\n");
    p += String("\r\n");

    print(p);
    print(ggaMessage);
    print("\r\n");
    LOG_INFO("[NTRIP] NTRIPClient sent GGA: %s", ggaMessage);
}

void NTRIPClient::setLastGGA(String gga) {
    lastGGA = gga;
}

String NTRIPClient::getLastGGA() {
    return lastGGA;
}