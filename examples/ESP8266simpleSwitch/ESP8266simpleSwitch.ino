//version: 2021-10-11
/*
   //Example for controlling a simple heating relay on pin 5
   Web:  https://console.tabahi.tech/
   1 - Set your WiFi SSID and password
   2 - Set your USER_TOKEN and USER_SECRET. Copy from 'account' page on console.tabahi.tech
   3 - Create some variables using set_bool("heater", heater_state); or set_float("var_b", 123.65);
   4 - Upload the code, if internet is connected, then view the newly created node on console that is matched by MAC address

*/

#include <ESP8266WiFi.h>
#include <TabahiConsole.h>


#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK  "your-password"
#endif

const char* ssid     = STASSID;
const char* password = STAPSK;

#define TTC_server "console.tabahi.tech"
#define USER_TOKEN "6105085ec003160b5e93f000"
#define USER_SECRET "fffc40000d9111"
TTC Console(TTC_server, 2096, 44561, USER_TOKEN, USER_SECRET);

const int heater_pin = 5; //GPIO5, D1
String mac_id = "0000"; //will set in setup
long heartbeat = 30000;  //connection interval time
boolean heater_state = 0;
unsigned long conn_timer = 0;   //connection timer
boolean send_variables = false; //set true after recieving first load of variables from server
String node_token = "";

void setup()
{
  pinMode(heater_pin, OUTPUT);
  digitalWrite(heater_pin, !heater_state);
  delay(100);
  Serial.begin(115200);
  delay(500);

  Serial.print(F("\nMAC ID: "));
  mac_id = WiFi.macAddress();
  Serial.println(mac_id);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());


  Console.initialize();

  //Set initial values for variables:
  Console.set_long("heartbeat", heartbeat);
  Console.set_bool("heater", heater_state);


  Console.printVariables(); //print on Serial


  delay(10);

  Console.logln(F("Ready\n\n"));
  Console.CommitLogs(mac_id); //send logs to server using mac_id as identifier
}



void loop()
{

  if ((millis() - conn_timer) > heartbeat)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      WiFiClient TCPclient;

      if (node_token.length() == 24)
      {
        int n_vars = Console.runSync(&TCPclient, node_token, send_variables);

        if (n_vars >= 0) //check if variables sync was ok
        {
          if (Console.isValidType("heartbeat", 'l')) //check if a valid long 'heartbeat' is synced
            heartbeat = Console.get_long("heartbeat");  //load the latest value recieved from server

          if (Console.isValid("heater")) //check if a valid long 'heater' is synced
          {
            heater_state = Console.get_bool("heater");
            digitalWrite(heater_pin, !heater_state);
          }
          if (!send_variables) send_variables = true; // This is set 'true' after first sync to avoid sending initial values
        }

        delay(1); yield();

        //send a data row:

        Console.newDataRow();
        Console.push_int("heat", heater_state);
        /*
          Console.push_int("a", 10);
          Console.push_float("b", 10.6);
          Console.push_long("c", 9999999);
          Console.push_String("d", "test_ok");
        */
        if (Console.CommitData(&TCPclient, node_token)) Serial.println("Data sent");


        Console.log(F("Heap:")); Console.logln((long)ESP.getFreeHeap()); //check heap health
      }
      else
      {
        if (Console.Identify(&TCPclient, mac_id) != -1)
          node_token = Console.get_String(NODE_TOKEN);
      }

      delay(1); yield();

      Console.log("HB:");
      Console.logln(heartbeat);
      Console.CommitLogs(mac_id); //view logs in UDP monitor
    }
    else
    {
      Serial.println("Wifi disconnected");
    }

    conn_timer = millis();
  }

  delay(0);
}
