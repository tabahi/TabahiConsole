/*
 * https:/tabahi.tech
 * 
 * This code tries an OTA update if a newer version .bin is available on the console cloud.
 * If the device keeps restarting within 1 hour of running,
 * then a safe mode is used in which the code does nothing else 
 * but connects to wifi and tries to update to a newer verions if available 

 * Last checked on ESP32S3 2024-07-16
 */
 
 //DONT FORGET TO USE PARTITION Scheme WITH OTA (i.e., don't select Huge APP, no OTA)
#include <TabahiConsole.h>
#define myVersion "v2" //version name for this binary. 

const char* ssid     = "Wifi name";
const char* password = "Wifi pass";

#define TTC_server "api.tabahi.tech"
#define USER_TOKEN "6223338df64144aac74a3622" //copy from account
#define USER_SECRET "Deu9DqvSS6pbNuIoI43aCh" //copy from account
#define DEBUG_TTC 1 //set to 1 to print verbose info on Serial

TTC Console(TTC_server, 2096, 44561, USER_TOKEN, USER_SECRET, DEBUG_TTC);

String mac_id = ""; //will set in setup. Used for node identification.
unsigned long heartbeat = 10000;
unsigned long timer_ = 0;

void setup()
{
  Serial.begin(115200);

  WiFi.disconnect(true); // delete old config
  delay(1000);
  WiFi.begin(ssid, password);

  mac_id = WiFi.macAddress();
  Serial.print(F("\nMAC ID: "));
  Serial.println(mac_id);

  Console.initialize();
  //Console.set_NODE_TOKEN("61800000000000000000000"); //if NODE_TOKEN (NT) is already known then 'Console.Identify()' can be skipped

  Console.ClearAllVariables();
  Console.printVariables(); //print on Serial
  Serial.println();
  Serial.println("Wait for WiFi... ");
  
  delay(1000);//wait for wifi to connect
  
}



void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {

    if (millis() - timer_ > heartbeat)
    {
      Serial.println("WiFi ok");
      
      Console.logln(myVersion);
      WiFiClient TCPclient;

      if (Console.node_token_valid == false)
      {
        int idn_status = Console.Identify(&TCPclient, mac_id);
        if (idn_status == 1)
        {
          Serial.print("NT: "); Serial.println(Console.NT);
        }
        else
        {
          Serial.printf("IDN failed %d\n", idn_status);
        }
      }

      else
      {
        int n_vars = Console.runSync(&TCPclient);

        if (n_vars >= 0) //check if variables sync was ok
        {

          //Serial.println(Console.time()); //time is synced with UTC epoch
          if (Console.isValidType("heartbeat", 'u')) //check if a valid long 'heartbeat' is synced
            heartbeat = Console.get_ulong("heartbeat");  //load the latest value recieved from server

          Console.printVariables();


          Console.set_String("version", myVersion);
          Console.set_time("testTime", Console.realtime());
          Console.set_geo("geo1", "-12.23232", "65.545");


          if (Console.isValid("test_var")) //check if a valid long 'heater' is synced
          {
            Serial.print("test_var=");
            Serial.println(Console.get_int("test_var"));
          }
        }

        delay(1); yield();

        //send a data row:

        Console.newDataRow(); //new row with a current timestamp

        Console.push_ulong("a",  4294967294);
        Console.push_float("b", 3402565.123);
        Console.push_float("x", -3402.681);
        Console.push_long("c", -2147483647);
        Console.push_String("geo_loc", "1.223, -5.235"); //"using "geo" in the name will link to google maps
        if (Console.CommitData(&TCPclient) > 0) Serial.println("Data sent");

        Console.log(F("Heap:")); Console.logln((long) ESP.getFreeHeap()); //check heap health

        Console.CommitLogs(mac_id.c_str());//send all the logs to server

        check_for_update();
      }
      timer_ = millis();
    }
  }
  else
  {
    Serial.println("No WiFi");
    WiFi.disconnect(true);
    WiFi.begin(ssid, password); //try reconnecting
    delay(20000);
    
  }

}


void check_for_update()
{
  WiFiClient TCPclient;
  String latest_bin_link = Console.fetchUpdateURL(&TCPclient, TTC_server, ""); //update server can be different, the last argument is for identifier if NODE_TOKEN is not known

  if (latest_bin_link == "0") Console.logln("No update");
  else if (latest_bin_link == "ERROR") Console.logln(latest_bin_link);
  else if (latest_bin_link.length() > 0)
  {
    if (latest_bin_link.indexOf(myVersion) == -1)  //URL doesn't contains the current version name
    {
      Console.log("New version: ");
      Console.logln(latest_bin_link);
      Console.logln("Updating");
      Console.CommitLogs(mac_id.c_str());

      if (!Console.executeOTAupdate(latest_bin_link))
        Console.logln("Update Failed");
    }
    else Console.logln("Latest version");
  }
  Console.CommitLogs(mac_id.c_str());//send all the logs to server
}
