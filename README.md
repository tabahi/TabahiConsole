# Tabahi Console

![Version](https://img.shields.io/static/v1?label=v&message=1.2&color=success)
![Device](https://img.shields.io/static/v1?label=Device&message=ESP32&color=blueviolet)
![Device](https://img.shields.io/static/v1?label=Device&message=ESP8266&color=blueviolet)
![Device](https://img.shields.io/static/v1?label=Device&message=Arduino&color=blueviolet)
![Language](https://img.shields.io/github/languages/top/tabahi/TabahiConsole)

[https://tabahi.tech](https://tabahi.tech/)

Example for ESP32 and ESP8266:
```cpp
#include <TabahiConsole.h>

const char* ssid     = "WiFi Name";
const char* password = "WiFi Pass";

#define TTC_server "api.tabahi.tech" //api.tabahi.tech
#define USER_TOKEN "6223338df64144aac74a3622"
#define USER_SECRET "Deu9DqvSS6pbNuIoI43aCh"
#define DEBUG_TTC 1 //set to 1 to print verbose info on Serial

TTC Console(TTC_server, 2096, 44561, USER_TOKEN, USER_SECRET, DEBUG_TTC);

String mac_address = ""; //will set in setup. Used for node identification.
int increment = 0;


void setup()
{
  Serial.begin(115200);
  WiFi.disconnect(true); // delete old wifi config
  delay(1000);
  WiFi.begin(ssid, password);

  mac_address = WiFi.macAddress();
  Serial.print(F("\nMAC: "));
  Serial.println(mac_address);

  Console.initialize();

  delay(5000); //wait 5 sec for Wifi to connect
}



void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    if (Console.node_token_valid == false) //Need to get a Node Token before anything else
    {
      WiFiClient TCPclient;
      int idn_status = Console.Identify(&TCPclient, mac_address);  //Identify Node Token using mac address
      if (idn_status == 1) {
        Serial.print("NT: ");
        Serial.println(Console.NT);
      }
      else {
        Serial.printf("Identification failed %d\n", idn_status);
      }
    }

    if (Console.node_token_valid == true)
    {
      sync_variables();
      unsigned long heartbeat = Console.get_ulong("heartbeat");

      //check if a there is a variable 'example'
      if (Console.isValid("example"))
        bool example = Console.get_bool("example");

      //setting a variable to a new value
      increment++;
      Console.set_int("increment", increment);

      push_data();

      Console.log("UTC time: ");
      Console.logln(Console.realtime()); //time is synced with UTC epoch in total seconds (unsigned long)

      Console.logln("Done"); //view this in the UDP monitor
      Console.CommitLogs(mac_address.c_str()); //send all the monitor logs to server, using mac address as the identifier
    }

    delay(30000);
  }
  else
  {
    Serial.print("Waiting for WiFi\t");
    Serial.println(ssid);
    WiFi.disconnect(true);
    WiFi.begin(ssid, password); //try reconnecting
    delay(10000); //wait 10 sec
  }
}

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

  return n_vars;
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

```

## Functions
Start by initializing the class

`TTC Console(TTC_server, TTC_port, UDP_port, USER_TOKEN, USER_SECRET, DEBUG_TTC);`
then call the following during `setup()`:

#### `void initialize(void)`

#### `void initialize(const char *TTC_server, int TTC_port, int UDP_port, const char *USER_TOKEN, const char *USER_SECRET, bool enable_debug)`

Initializes the `TTC` class object if it's already configured. Otherwise it with a new configuration.


#### `int Identify(TCPClientObj *TCPclient, String mac_id_str)`

Each device is assigned a Node Token by the console. Token can be generated manually on the console and pre-configured, or the device can use any unique ID such as MAC address to identify itself and get a Node Token (NT).


#### `int runSync(TCPClientObj *TCPclient)`

Sync function sends the current value of variables (if any) and receives the latest value from the cloud. In case of a conflict, the value from the server is preferred. But if the value changes between two syncs, then the value from the device is preferred unless the variable is set to constant.

To trigger a script on the cloud after the sync use:

#### `int runSync(TCPClientObj *TCPclient, String script_token, String args)`

where `script_token` is a 24 character token assigned to the script on the console. And `args` are the arguments to be passed to the script. e.g., example=ABC0000000, a=123. Leave empty as "" if there are no arguments.


