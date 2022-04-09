//version: 2021-09-23
/*
   Web:  https://tabahi.tech/
   Wifi settings:
   Connect to Wifi named "myESP_8546498", pass: pass_ESP
   Then go to the URL http://192.168.1.1
   Login: admin, pass_ESP
   Set Wifi SSID and password
   Wait for the restart
   Get ESP8266webadmin library from: https://github.com/tabahi/ESP8266-Web-Admin

   Then check the logs in the web console

*/
#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266webadmin.h>
#include <TabahiConsole.h>

#define THIS_VERSION  "202109a30"  //Version value of this binary
#define VERSION_VAR   "version" //'name' of the version variable as defined in console, if the value this 'version' is not the same as the latest then it will update
#define HEARTBEAT     "heartbeat" //connection interval ms, all the data and variables are synced to the server after this time. 
#define ENABLE_UPDATE "update"  //set it's value 1 in console to update to the latest version using the bin link given on Node configuration panel

#define ESP_config_reset_pin 0
ESP8266webadmin ESPConfig("myESP", 80, ESP_config_reset_pin, false, "fallback_wifi", "fallback_pass", true);

WiFiEventHandler stationConnectedHandler;
WiFiEventHandler stationDisconnectedHandler;

#define TTC_server "console.tabahi.tech"
#define USER_TOKEN "61554cc872acbfeaa98282b8"
#define USER_SECRET "749b098fbd3f45"
TTC Console(TTC_server, 2096, 44561, USER_TOKEN, USER_SECRET);


unsigned int AP_StationConnected = 0;
String mac_id = "0000"; //will set in setup
long heartbeat = 30000;  //connection interval time
unsigned long conn_timer = 0;   //connection timer

boolean first_sync = true; //set false after recieving first load of variables from server



void setup()
{
  delay(100);
  Serial.begin(115200);
  delay(500);

  Console.logln(F("\n\n\nStart\n"));
  Serial.print(F("\nMAC ID: "));

  mac_id = WiFi.macAddress();
  Serial.println(mac_id);

  Console.initialize();
  String version_string = THIS_VERSION;

  //Set initial values for variables:
  Console.set_String(VERSION_VAR, &version_string);
  Console.set_bool(ENABLE_UPDATE, 1);
  Console.set_long(HEARTBEAT, heartbeat);
  Console.printVariables();

  Console.log(F("VER: ")); Console.logln(THIS_VERSION); //log the version to UDP monitor


  if (ESPConfig.initialize() == AP_MODE)
  {
    stationConnectedHandler = WiFi.onSoftAPModeStationConnected(&onStationConnected);
    stationDisconnectedHandler = WiFi.onSoftAPModeStationDisconnected(&onStationDisconnected);
	ESPConfig.Start_HTTP_Server();
  }
  else AP_StationConnected = 0;

  ESPConfig.print_settings();
  delay(10);

  Console.logln(F("Ready\n\n"));
}



void loop()
{
  ESPConfig.handle(600000);  //HTTP configuration server remains active for first 10 minutes (600000 ms) during client mode

  delay(0);
  unsigned long millis_now =  millis();


  if ((AP_StationConnected == 0) && (ESPConfig.wifi_connected) && (ESPConfig.ESP_mode == CLIENT_MODE))
  {
    if (((millis_now - conn_timer) > heartbeat) || (conn_timer == 0) )
    {
      Console.log("\n\nt=");
      Console.logln(int(millis_now / 1000));
      Console.CommitLogs(mac_id); //send logs to server using mac_id as identifier

      delay(1); yield();


      String node_token = Identify_Node_Token(mac_id);

      if (node_token.length() == 24)
      {
        WiFiClient TCPclient;
        int n_vars = Console.runSync(&TCPclient, node_token, !first_sync); //!first_sync = Don't send initial values of variables until after first sync
		if(n_vars!=-1)
		{
			heartbeat = Console.get_long(HEARTBEAT);  //load the variables recieved from server such as timer etc

			if (first_sync)
			{
			  first_sync = false;
			  check_update(node_token);
			  checkWeatherForecast();
			}
			delay(1); yield();

			Console.newDataRow();
			Console.push_int("a", 10);
			Console.push_float("b", 10.6);
			Console.push_long("c", 9999999);
			Console.push_String("d", "yooooooo");
			if (Console.CommitData(&TCPclient, node_token)) Serial.println("Data sent");

			Console.log(F("Heap:")); Console.logln((long)ESP.getFreeHeap()); //check heap health
		}
      }

      delay(1); yield();

      Console.log("HB:");
      Console.logln(heartbeat);
      Console.CommitLogs(mac_id);

      conn_timer = millis_now;
    }
  }
  else
  {
    conn_timer = 0;
  }

  delay(0);

  // Don't use big delays in loop()
}




String Identify_Node_Token(String identifier)
{
  String node_token = Console.isValid(NODE_TOKEN) ? Console.get_String(NODE_TOKEN) : ""; //get NODE_TOKEN from variables memory
  if (node_token.length() != 24)
  {
    //get a NODE_TOKEN using MAC address as identifier

    Serial.println(F("Identifying..."));

    WiFiClient TCPclient;
    int n_vars = Console.Identify(&TCPclient, identifier);
    if (n_vars > 0) //got some new variables
      node_token = Console.get_String(NODE_TOKEN);

    if ((!Console.isValid(NODE_TOKEN)) || (node_token.length() != 24))
      Serial.println(F("Failed to get Node Token"));
    else
      Serial.print("NT:"); Serial.println(node_token);
  }
  return node_token;
}













//Wifi Event functions, ignore
void onStationConnected(const WiFiEventSoftAPModeStationConnected& evt)
{
  AP_StationConnected++;
  Serial.print(AP_StationConnected);
  Serial.println(F(" WIFI ST CONN"));
}

void onStationDisconnected(const WiFiEventSoftAPModeStationDisconnected& evt)
{
  AP_StationConnected--;
  Serial.print(AP_StationConnected);
  Serial.println(F(" WIFI ST DISCONN"));
}
