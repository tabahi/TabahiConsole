//Last update: 30-05-2023, tested on ESP32
/*
   Web:  https://tabahi.tech/
   Wifi settings:
   Connect to Wifi named "myESP_8546498", pass: pass_ESP
   Then go to the URL http://192.168.1.1
   Login: admin, pass_ESP
   Set Wifi SSID and password
   Wait for the restart
   Get ESP8266webadmin library from: https://github.com/tabahi/ESP-Wifi-Config

   Then check the logs in the web console

*/
#include <ESPWifiConfig.h>
#include <TabahiConsole.h>


//WiFi Config
#define ESP_config_reset_pin 0	//GPIO0, D3 on NodeMCU
#define debug_mode 1  //ESPWifiConfig will print information to Serial in debug mode
#define overwrite_fallback false //true: overwrite this fallback wifi as primary wifi in memory. false: use this wifi as a fallback wifi when the configured wifi is not available
ESPWifiConfig ESPConfig("myESP", 80, ESP_config_reset_pin, overwrite_fallback, "lalala", "lalala420", debug_mode);


#define TTC_server "api.tabahi.tech" //api.tabahi.tech
#define USER_TOKEN "6223338df64144aac74a3622" //copy from account
#define USER_SECRET "Deu9DqvSS6pbNuIoI43aCh" //copy from account

#define DEBUG_TTC 1 //set to 1 to print verbose info on Serial

TTC Console(TTC_server, 2096, 44561, USER_TOKEN, USER_SECRET, DEBUG_TTC);


String mac_address = ""; //will set in setup. Used for node identification.
unsigned long heartbeat = 10000;

String node_token = ""; //hold the node_token string character

unsigned long conn_timer = 0;   //connection timer



void setup()
{
  delay(100);
  Serial.begin(115200);
  delay(500);

  Console.logln(F("\n\n\nStart\n"));

  mac_address = WiFi.macAddress();
  Serial.print(F("\nMAC: "));
  Serial.println(mac_address);

  Console.initialize();


  if (ESPConfig.initialize() == AP_MODE)
  {
		Serial.println("AP mode, WiFi cofiguration needed");
		ESPConfig.Start_HTTP_Server(60000); //HTTP web server remains active for first 10 minutes (600000 ms) if it's switched into CLIENT_MODE. Set 0 to run it perpetually. It's perpetually ON in AP_MODE.
  }

  ESPConfig.print_settings();
  delay(10);

  Console.logln(F("Ready\n\n"));
}



void loop()
{
  ESPConfig.handle(10000);  //non-blocking function, 100000 is an interval for reconnection if disconnected

  delay(0);
  unsigned long millis_now =  millis();


  if ((ESPConfig.wifi_connected) && (ESPConfig.ESP_mode == CLIENT_MODE))
  {
    if (((millis_now - conn_timer) > heartbeat) || (conn_timer == 0) )
    {

      //Get a Node Token from the cloud server before anything else
      if (Console.node_token_valid == false) 
      {
         //Identify Node Token using mac address
        WiFiClient TCPclient;
        int idn_status = Console.Identify(&TCPclient, mac_address); 
        if (idn_status == 1)
        {
          Serial.print("NT: "); Serial.println(Console.NT);
          //Warn: DON'T SEND NT OVER UDP (INSECURE). CommitLogs sends logs over UDP without encryption. All other functions use TCP with encryption
        }
        else
        {
          Serial.printf("IDN failed %d\n", idn_status);
        }
      }

      else //if already have the node token
      {
        sync_variables(); //sync, update, add, edit variables, see below

        push_data();    //push data to tables, see below

        Console.log(F("Heap:")); Console.logln((long)ESP.getFreeHeap()); //check heap health
        //print some logs on UDP monitor:
        Console.log("UTC time: "); //will show on UDP monitor on console.tabahi.tech
        Console.logln(Console.realtime()); //time is synced with UTC epoch in total seconds (unsigned long)
        Console.logln("Done");
        Console.CommitLogs(mac_address.c_str()); //send all the monitor logs to server, using mac address as the identifier
        //must call CommitLogs() after log() to send logs to the server.
      }
      
      conn_timer = millis_now;

    }
  }
  else
  {
    //can't do much without internet
    conn_timer = 0;
  }

  delay(0);

  // Don't use big delays in loop()
}



int increment_example_var = 0; // an example variable

int sync_variables()
{
  WiFiClient TCPclient;
  int n_vars = Console.runSync(&TCPclient);

  if (n_vars >= 0) //check if variables sync was ok
  {
    Console.logln("Sync: OK");
    Console.printVariables(); //print all the vairables on Serial
  }
  else
    Serial.printf("Sync failed, status: %d \n", n_vars);

  if (Console.isValidType("heartbeat", 'u'))
    heartbeat = Console.get_ulong("heartbeat");

  //check if a there is a variable 'example'
  if (Console.isValidType("example", 'b'))
    bool example = Console.get_bool("example");

  //setting a variable to a new value
  increment_example_var++;
  Console.set_int("increment", increment_example_var); //this new value will show on the console after the next sync cycle

  return n_vars;  //returns number of synced variables. Error if it's negative.

  /* ERRORS:
  ERR_FAILED_CONN -1      //Connection failed to the server
  ERR_FAILED_CONN_10x -10 //10 times consistent failure. Probably no internet connection.
  ERR_NO_NODE_TOKEN -2    //NT isn't identified, call Console.Identify(&TCPclient, mac_address); for a new NT.
  ERR_ACK_FAILED -3       //usually due to wrong account security details. UDP Console.log() might still work because that's not encrypted.
  ERR_DATA_PARSE -4       //usually due to wrong TTC or JSON syntax, try removing quotations.
  */
}










void push_data()
{
  //create a data row:
  Console.newDataRow(); //new row with a current timestamp
  Console.push_ulong("a",  123);
  Console.push_float("b", 456.789);
  Console.push_String("c_geo", String("1.223") + "," + String("-5.235"));  //heading including 'geo' will link to google maps

  WiFiClient TCPclient;
  //Data is not sent until 'CommitData' is called.
  if (Console.CommitData(&TCPclient) > 0) Console.logln("Data sent");

}
