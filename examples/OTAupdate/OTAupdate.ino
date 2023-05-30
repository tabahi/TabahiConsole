/*
 * https:/tabahi.tech
 * 
 * This code tries an OTA update if a newer version .bin is available on the console cloud.
 * If the device keeps restarting within 1 hour of running,
 * then a safe mode is used in which the code does nothing else 
 * but connects to wifi and tries to update to a newer verions if available 
 */
 
#include <TabahiConsole.h>
#define EE_SIZE 4096
#define SAFE_RUN_MIN_TIME 3600000 //1 hour
#define MIN_CODE_FAILURES 10  //if it restarts 10 times within the first hour of running then safe mode is activated on the next run
#define myVersion "version1" //version name for this binary

const char* ssid     = "WiFi Name";
const char* password = "WiFi Pass";

#define TTC_server "api.tabahi.tech"
#define USER_TOKEN "6225868df6412032c74a3698"
#define USER_SECRET "Deu9DqvSS6pbNuIoI43aCh"
#define DEBUG_TTC 1 //set to 1 to print verbose info on Serial

TTC Console(TTC_server, 2096, 44561, USER_TOKEN, USER_SECRET, DEBUG_TTC);

String mac_id = ""; //will set in setup. Used for node identification.
unsigned long heartbeat = 10000;

bool safe_mode = false;

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


  //Don't do anything except update in safe mode because something is causing unintended resets
  if (start_in_safe_mode())
  {
    //safe mode. Do nothing except connect WiFi and wait for the next version
    safe_mode = true;
  }
  else
  {
    safety_on(); //turn on safety before critical funtions
    Console.ClearAllVariables();
    Console.printVariables(); //print on Serial
    Serial.println();
    Serial.println("Wait for WiFi... ");
    safety_off(); //turn off safety after critical funtions
    delay(1000);//wait for wifi to connect
  }
}



void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("WiFi ok");
    if (safe_mode)
    {
      Console.logln("SAFE MODE");
      Console.logln(myVersion);
      Console.CommitLogs(mac_id.c_str());

      WiFiClient TCPclient;
      if (Console.Identify(&TCPclient, mac_id) == 1)
      {
        try_update();
      }
      delay(10000);
    }
    else //regular mode
    {
      safety_on(); //turn on safety before critical funtions
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

          Serial.printf("Msgs in inbox: %d\n", Console.inbox);

          Serial.printf("Time:\t %d-%d-%d %d:%d \tweekday:%d\n", Console.year(), Console.month(), Console.date(), Console.hour(), Console.minute(), Console.weekday());

          Console.printVariables();


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
        if (Console.CommitData(&TCPclient) > 0) Serial.println("Data sent");

        Console.SendJSON(&TCPclient, "{\"row1\":{\"data1\": 90, \"data2\": 0.211, \"data3\": \"chill\"}, \"time\":" + String(Console.realtime()) + "}");
        Console.SendJSON(&TCPclient, "{\"row2\":{\"data1\": 80, \"data2\": 0.456, \"data3\": \"pill\"}}"); //server adds it's own timestamp

        Console.log(F("Heap:")); Console.logln((long) ESP.getFreeHeap()); //check heap health

        Console.CommitLogs(mac_id.c_str());//send all the logs to server

        try_update();
      }
      safety_off(); //turn off safety after critical funtions
      if (millis() > SAFE_RUN_MIN_TIME) safety_assured(); //after 1 hour of running set fail count to zero


      delay(heartbeat);
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



bool start_in_safe_mode()
{
  EEPROM.begin(EE_SIZE);
  uint8_t code_fail_count = EEPROM.read(0);
  uint8_t code_safety_on = EEPROM.read(1);

  if (code_safety_on == 1) //it was reset when running a critical part of the code
  {
    code_fail_count++;
    EEPROM.write(0, code_fail_count);
  }
  EEPROM.end();
  Console.logln("Code fails: " + String(code_fail_count));

  if ((code_fail_count >= MIN_CODE_FAILURES) && (code_fail_count < MIN_CODE_FAILURES+2)) return true;
  else return false;
}


void safety_on()
{
  EEPROM.begin(EE_SIZE);
  EEPROM.write(1, 1);
  EEPROM.end();
}

void safety_off()
{
  EEPROM.begin(EE_SIZE);
  EEPROM.write(1, 0);
  EEPROM.end();
}

void safety_assured()
{
  EEPROM.begin(EE_SIZE);
  EEPROM.write(0, 0);
  EEPROM.end();
}


void try_update()
{
  safety_off();
  safety_assured();
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
}
