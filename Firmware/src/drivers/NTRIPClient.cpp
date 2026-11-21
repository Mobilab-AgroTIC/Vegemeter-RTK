#include"NTRIPClient.h"
#include "common/logger.h"
bool NTRIPClient::reqSrcTbl(char* host,int &port)
{
  if(!connect(host,port)){
    LOG_WARN("[NTRIP] Connexion à %s impossible", host);
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
        LOG_WARN("[NTRIP] Pas de réponse du caster (table des sources)");
        stop();
        return false;
     }
     vTaskDelay(pdMS_TO_TICKS(10));
  }
  char buffer[12];
  readLine(buffer,sizeof(buffer));
  if(strstr(buffer,"SOURCETABLE 200 OK")!=NULL)
  {
    LOG_DEBUG("[NTRIP] Table des sources reçue");
    LOG_DEBUG("[NTRIP] Buffer: %s", buffer);
    return false;
  }
  return true;
}

bool NTRIPClient::reqRaw(char* host,int &port,char* mntpnt,char* user,char* psw)
{
    if(!connect(host,port)){
        LOG_DEBUG("[NTRIP] Connexion TCP à %s:%d impossible", host, port);
        return false;
    }
    String p="GET /";
    String auth="";
    LOG_DEBUG("[NTRIP] Demande du flux RTCM (mountpoint %s)", mntpnt);
    
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

        p = p + String("Authorization: Basic ");
        p = p + auth;
        p = p + String("\r\n");
    }
    p = p + String("\r\n");
    print(p);
    unsigned long timeout = millis();
    while (available() == 0) {
        if (millis() - timeout > 20000) {
            LOG_DEBUG("[NTRIP] Pas de réponse du caster (timeout 20 s)");
            stop(); // sinon connected() resterait vrai sans flux RTCM
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    char buffer[50];
    readLine(buffer,sizeof(buffer));
    if(strncmp((char*)buffer,"ICY 200 OK",10))
    {
      // Réponse inattendue (ex. "HTTP/1.1 401 Unauthorized", mountpoint inconnu...)
      String resp(buffer);
      resp.trim();
      LOG_THROTTLED(LOG_ERROR, LOG_THROTTLE_MS, "[NTRIP] Caster a refusé la connexion : \"%s\"", resp.c_str());
      stop();
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
        LOG_WARN("[NTRIP] Client non connecté, reconnexion...");
        if (!connect(host, port)) {
            LOG_WARN("[NTRIP] Reconnexion échouée");
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
    LOG_DEBUG("[NTRIP] GGA envoyée : %s", ggaMessage);
}

void NTRIPClient::setLastGGA(String gga) {
    lastGGA = gga;
}

String NTRIPClient::getLastGGA() {
    return lastGGA;
}