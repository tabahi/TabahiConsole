
/*
   Inside this text file there must a URL for the latest version binary file
   The binary filename must contain the latest version name
   "http://console.tabahi.tech/assets/update/{Node_Token}/version.txt"
   Example of text inside version.txt file :
  http://console.tabahi.tech/assets/update/{Node_Token}/202109b14.bin

  where 202103b06.bin is the binary file of latest compiled code of version name '202103b05'

*/


void check_update(String nt)
{
  if  (Console.get_bool(ENABLE_UPDATE))
  {
    delay(1); yield();
    WiFiClient TCPclient;
    if (try_update(&TCPclient, &nt, THIS_VERSION)) Serial.println(F("Update check ok"));
    else Serial.println(F("Update check fail"));
  }
}


uint8_t try_update(ClientObj *TCPclient, String *nt, String curren_version)
{

  uint8_t update_status = 10;

  delay(1);
  String latest_v = fetch_latest_version_URL(TCPclient, nt);
  delay(0);
  if (latest_v.length() > 1)
  {
    int http_i = latest_v.indexOf("http://");
    int bin_i = latest_v.indexOf(".bin");
    if ((http_i >= 0) && (bin_i > http_i))
    {
      latest_v = latest_v.substring(http_i, bin_i + 4);
      Serial.println(latest_v);
      if (latest_v.indexOf(curren_version) >= 0)
      {
        Console.logln(F("[U] Latest version"));
        
        update_status = 1;
      }
      else
      {
        Console.logln(F("[U] Needs Update"));
        update_status = 0;
#if BOARD_ESP
        ESP8266_update(latest_v);
#endif
      }
    }
    else
    {
      Console.logln(F("[U] Update not available"));
      update_status  = 2;
    }
  }

  return update_status;
}


String fetch_latest_version_URL(ClientObj *TCPclient, String *nt)
{
  delay(1); yield();
  //WiFiClient client;

  String payload = "";
  Console.logln(F("[U] checking..."));
  if (!TCPclient->connect(TTC_server, 80))
  {
    Console.logln(F("[U] Conn Failed"));
    return payload;
  }
  else
  {
    unsigned long up_v_start = millis();

    delay(0);
    TCPclient->print(F("GET /assets/update/"));
    //String nt = get_String(NODE_TOKEN);
    if (nt->length() > 0) TCPclient->print((*nt));
    else TCPclient->print("0");
    TCPclient->println(F("/version.txt HTTP/1.1"));

    TCPclient->print(F("Host: "));
    TCPclient->println(TTC_server);
    TCPclient->println(F("Cache-Control: no-cache"));
    TCPclient->println(F("Connection: close"));
    TCPclient->println();

    while ((millis() - up_v_start) < 20000)
    {
      while (TCPclient->available())
      {
        char c = TCPclient->read();
        payload += c;
      }
      if (payload.indexOf(".bin") >= 0) break;
      else if (payload.indexOf("NOT FOUND") >= 0) break;
      else if (payload.indexOf("http") > 0) payload = payload.substring(payload.indexOf("http"));
      else if (payload.indexOf("</") > 0) payload = "";
      else if (payload.length() > 90) payload = payload.substring(10);
    }
    TCPclient->stop();
  }
  return payload;
}


void ESP8266_update(String updateURL)
{
  yield();
  Serial.println("Updating");
  delay(1);
  WiFiClient up_client;
  t_httpUpdate_return ret = ESPhttpUpdate.update(up_client, updateURL);

  switch (ret)
  {
    case HTTP_UPDATE_FAILED:
      Console.log(F("HTTP_UPDATE_FAILD Error"));
      Console.log(ESPhttpUpdate.getLastError());
      Console.logln(ESPhttpUpdate.getLastErrorString());
      //Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Console.logln(F("HTTP_UPDATE_NO_UPDATES"));
      break;

    case HTTP_UPDATE_OK:
      Serial.println(F("HTTP_UPDATE_OK"));
      break;
  }
  delay(1);
}
