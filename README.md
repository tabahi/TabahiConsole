# Tabahi Console

![Version](https://img.shields.io/static/v1?label=v&message=1.2&color=success)
![Device](https://img.shields.io/static/v1?label=Device&message=ESP32&color=blueviolet)
![Device](https://img.shields.io/static/v1?label=Device&message=ESP8266&color=blueviolet)
![Device](https://img.shields.io/static/v1?label=Device&message=Arduino&color=blueviolet)
![Language](https://img.shields.io/github/languages/top/tabahi/TabahiConsole)


An arduino Library for ESP32 and ESP8266 cloud IoT interface. Easily control and manage your devices using the web console at:
[https://tabahi.tech](https://tabahi.tech/)


## Initializing


Start by initializing the class

`TTC Console(TTC_server, TTC_port, UDP_port, USER_TOKEN, USER_SECRET, DEBUG_TTC);`
then call the following during `setup()`:


```cpp
#include <TabahiConsole.h>

const char* ssid     = "WiFi Name";
const char* password = "WiFi Pass";

#define TTC_server "api.tabahi.tech" //api.tabahi.tech
#define USER_TOKEN "6223338df64144aac74a3622" //copy from account
#define USER_SECRET "Deu9DqvSS6pbNuIoI43aCh" //copy from account
#define DEBUG_TTC 1 //set to 1 to print verbose info on Serial

TTC Console(TTC_server, 2096, 44561, USER_TOKEN, USER_SECRET, DEBUG_TTC);

String mac_address = ""; //will set in setup. Used for node identification.

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
  /* Or reinitialize with different settings:
  Console.initialize(const char *TTC_server, int TTC_port, int UDP_port, const char *USER_TOKEN, const char *USER_SECRET, bool enable_debug);
  */
}
```



## Device Identification

Each device is assigned a Node Token by the console. Token can be generated manually on the console and pre-configured, or the device can use any unique ID such as MAC address to identify itself and get a Node Token (NT).

Automatically registering a new NT using the `mac_address` or any unique identifier so that you can differentiate one device from another:

```cpp
if (WiFi.status() == WL_CONNECTED)
{
  if (Console.node_token_valid == false) //Need to get a Node Token before anything else
  {
    WiFiClient TCPclient;

    //Identify Node Token using mac address
    //int Identify(TCPClientObj *TCPclient, String mac_id_str)
    int idn_status = Console.Identify(&TCPclient, mac_address);

    if (idn_status == 1) {
      Serial.print("Got NT: ");
      Serial.println(Console.NT);
    }
    else {
      Serial.printf("Identification failed %d\n", idn_status);
    }
  }
}
```

If you already added a device manually on the web console then set the assigned Node Token as:

```cpp
Console.set_NODE_TOKEN("61800000000000000000000"); //see the Configure tab for NT after adding a device
```

The mac address is the easiest identifier you can read and use for differentiating devices without changing the code for each device. It's not necessarily required by the console. If two of your devices are using the same identifier then the console will consider them as a single device. Once a Node Token is assigned then the mac address (or unique ID) isn't needed for variables and data syncing. 

UDP monitor doesn't use Node Token, instead it uses any String name you pass during `Console.CommitLogs("name_for_UDP_monitor");`. We use the mac address to make sure that each device shows up with a different name.

## Syncing

Syncing is performed over a secure TCP protocol. It first sends the values from the device's memory, then fetches the latest values from the console cloud. Optionally, you can also trigger a script on console after the sync.


```cpp
WiFiClient TCPclient;
int n_vars = Console.runSync(&TCPclient); //returns >=0 number of variables if no error


if (n_vars >= 0) //check if variables sync was ok
{
  Console.logln("Sync: OK");
  Console.printVariables(); //print all the vairables on Serial
}
else
  Serial.printf("Sync failed, status: %d \n", n_vars);


//parse the synced variables
unsigned long heartbeat = Console.get_ulong("heartbeat");

//check if a there is a bool variable 'example'
if (Console.isValidType("example", 'b'))
  bool example = Console.get_bool("example");

//setting a variable to a new value
int new_var = 123;
Console.set_int("new_var", new_var);
//this new value will show on the console after the next sync cycle

```

In the case of a conflict, the value from the console is preferred. But if the value changes on both sides, on the console AND on the device between two syncs, then the value from the device is preferred unless the variable is set to constant. There is a 2 minutes grace period for the console to yield when the device changes to a new value while the user has recently updated a new value in console. In other words, the new value that the user has set on the console will be discarded after 2 minutes because the device keeps on updating to a newer value. To avoid this, don't update the value on the console if the device is programmed to always set to a newer value between two syncs.

To trigger a script on the cloud after the sync use:

```cpp
  WiFiClient TCPclient;
  int n_vars = runSync(&TCPclient, "script_token", "arg1=abcd, arg2=123");
```

where `script_token` is a 24 character token assigned to the script on the console. And `args` are the arguments to be passed to the script. e.g., `val=ABC0000000, a=123`. Leave empty as `""` if there are no arguments.



## Data Logging

Data is inserted into the tables using a timestamp. Timestamp is created when `Console.newDataRow()` is called. Unlike synced variables, `Console.push_` functions are a one-way traffic. The device gets an acknowledgement upon the successful data insertion but it can't read back the data. The limit of data rows is less than variable writes on the console but users can delete the old data on the console to make space for the newer data.

Example usage for `push_float`,
`push_int`,
`push_long`,
`push_ulong`,
`push_String`:

```cpp
  //create a data row:
  Console.newDataRow(); //new row with a current timestamp
  Console.push_int("integer",  99);
  Console.push_ulong("a",  123);
  Console.push_float("b", 456.789);
  Console.push_String("c_geo", String("1.223") + "," + String("-5.235"));  //headings including 'geo' will link to google maps

  WiFiClient TCPclient;
  //Data is not sent until 'CommitData' is called.
  if (Console.CommitData(&TCPclient) > 0) Console.logln("Data sent");
```

Alternatively you can push data in JSON format with a single command:

```cpp
Console.SendJSON(&TCPclient, "{\"row1\":{\"data1\": 90, \"data2\": 0.211, \"data3\": \"chill\"}, \"time\":" + String(Console.realtime()) + "}");


Console.SendJSON(&TCPclient, "{\"row2\":{\"data1\": 80, \"data2\": 0.456, \"data3\": \"pill\"}}"); //server adds it's own timestamp if no epoch 'time' is given.
//Don't make rows too big to handle for memory
//No need to CommitData after SendJSON

```


## Intra-Node Communication
Small messages (300 chars) can be sent from one device to another via the console cloud. The `runSync` also checks the number of messages in the inbox for a device:

```cpp
int n_vars = Console.runSync(&TCPclient);

if (n_vars >= 0) //check if variables sync was ok
{
  Serial.printf("Inbox count: %d \n", Console.inbox);
}

```

Then  read all the messages one-by-one in either top-to-bottom or from bottom-to-top manner

```cpp
while (Console.inbox > 0)
{
  String readMsg = Console.readMessage(&TCPclient, 't'); //'t' from top, 'b' from bottom
  /*
  Message format example:
  >12,6253dc60b1fdc14832d2c6ee,the message content

  */
  //parse time, sender and the content of the message:
  if (readMsg[0] == '>')
  {
    int comma_index = readMsg.indexOf(',');
    unsigned long seconds_ago = atol(readMsg.substring(1, comma_index).c_str());
    readMsg = readMsg.substring(comma_index + 1); comma_index = readMsg.indexOf(',');
    String sender_NT = readMsg.substring(0, comma_index);
    String message = readMsg.substring(comma_index + 1);

    Serial.print("Recieved: "); Serial.print(seconds_ago);
    Serial.print(" seconds ago, from "); Serial.println(sender_NT);
    Serial.print("Msg:"); Serial.println(message);
  }
  else
  {
    Serial.print("Error:"); Serial.println(readMsg);
  }
  Console.inbox--;
}
```

To send a message to other devices, use their NT or identifier as

```cpp
//Send message to others (@ mac_address, or use Node Token Console.NT or identifier)
int sendMsgStatus = Console.sendMessage(&TCPclient, mac_address, "yo, sup?");
Serial.print("Msg sent:\t"); Serial.println(sendMsgStatus); //1 = sent
```


## UDP Monitor

UDP monitor uses a simpler, faster, protocol without any encryption to deliver debugging or informational logs to the console. The mechanism for UDP only requires the correct `USER_TOKEN` to work, therefore can be used to debug other more sophisticated functionalities in case of failure.

```cpp
//print some logs on UDP monitor:
Console.log("UTC time: "); //will show on UDP monitor on console.tabahi.tech
Console.logln(Console.realtime());
Console.logln("This is an information");
Console.CommitLogs(mac_address.c_str()); //send all the monitor logs to server, using mac address as the identifier
//must call CommitLogs() after log() to send logs to the server.
```

The identifier for UDP monitor can be anything but it helps to keep devices separate from each other by using a unique hardware identifier such as MAC address. `log()` and `logln()` functions store the data in a temporary buffer which is sent to the console on `CommitLogs()`.

Note: Avoid sending critical information such as `USER_TOKEN` over the UDP Monitor because it's not encrypted. Whereas, all other features such as `runSync, CommitData and sendMessage` are encrypted for increased security.


## Weather

You can get the weather forecast and sun timing using the Latitude and Longitude for a location. The API uses the data provided by met.no, which provide data for local times around the globe.


```cpp

void checkWeatherForecast()
{
  //Should define at top of SettingsTTC.h :
  //#define WEATHER_HOURS_MAX 12
  //Then pass parameters as:
  String geo_lat = "60.362";
  String geo_lon = "5.013";
  int forecast_hours = 12;

  WiFiClient TCPclient;
  uint8_t hours_reported = Console.fetchWeather(&TCPclient, geo_lat, geo_lon, forecast_hours);
  Console.log(F("Got forecast for hr:\t")); Console.logln(hours_reported);

  for (int i = 0; i < forecast_hours; i++)
  {
    if (Console.forecast[i].temp != INVALID_VALUE)
    {
      Serial.print("Hour=");
      Serial.print(i);
      Serial.print('\t');
      Serial.print(Console.forecast[i].temp);
      Serial.print('\t');
      Serial.print(Console.forecast[i].humid);
      Serial.print('\t');
      Serial.print(Console.forecast[i].perc);
      Serial.print('\t');
      Serial.println(Console.forecast[i].symbol);
    }
  }

}
```

The forecast data for `WEATHER_HOURS_MAX` hours is held in the memory for later use, so that the device doesn't have to check the forecast again and again. The decrease or increase the number of forecast hours, edit the first line of `SettingsTTC.h`.

## Sunrise and Sunset

Some daylight dependent applications require the sunrise and sunset timings. Met.no API provides these timings in local time. The elevation of the sun at noon is also provided for solar tracking applications.


```cpp


void checkSunrise()
{
  String geo_lat = "51.500";
  String geo_lon = "-0.1475";

  WiFiClient TCPclient;
  uint8_t days_status = Console.fetchSunrise(&TCPclient, geo_lat, geo_lon);
  Console.log(F("days_status:\t")); Console.logln(days_status); //0 for failed, 1 for success

  Serial.print("Sunrise: ");
  Serial.print(Console.sunrise_hour); Serial.print(':'); Serial.print(Console.sunrise_minutes);
  Serial.print("\tNoon: ");
  Serial.print(Console.noon_hour); Serial.print(':'); Serial.print(Console.noon_minutes);
  Serial.print("\tSunset: ");
  Serial.print(Console.sunset_hour); Serial.print(':'); Serial.print(Console.sunset_minutes);
  Serial.print("\tNoon Elevation: ");
  Serial.print(Console.noon_angle);
  Serial.print("\tMoon phase: ");
  Serial.println(Console.moon_phase);
}

```


## OTA Update

A compiled binary `.bin` can be uploaded on the console for each device with a new version name for the device to check if their version is different therefore an update should be executed. In the following example, first `fetchUpdateURL` is used to get the link of the binary which will include the version name (if update available) in it. If the link doesn't include the version name of the currently running binary then the update is started using the new binary link with `executeOTAupdate`.

```cpp
#define myVersion "version1" //version name for this binary

void try_update()
{
  WiFiClient TCPclient;
  String latest_bin_link = Console.fetchUpdateURL(&TCPclient, TTC_server, "");
  //update server can be different (should be HTTP), the last argument is for identifier if NODE_TOKEN is not known

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
```
See example `OTAupdate.ino` for updating with a safe mode in case the updated version keeps on restarting.




## TTC Console Class

Use all this functions with the class prefix. e.g., `Console.get_int`
### Variables

#### Variable: int
```cpp
int16_t get_int(char *key_name);
bool set_int(char *key_name, int16_t val);
```

#### Variable: float
```cpp
float get_float(char *key_name);
bool set_float(char *key_name, float val);
```

#### Variable: long
```cpp
long get_long(char *key_name);
bool set_long(char *key_name, long val);
```

#### Variable: ulong
```cpp
unsigned long get_ulong(char *key_name);
bool set_ulong(char *key_name, unsigned long val);
```

#### Variable: bool
```cpp
bool get_bool(char *key_name);
bool set_bool(char *key_name, bool val);
```

#### Variable: String
```cpp
String get_String(char *key_name);
bool set_String(char *key_name, String val);

//or use a pointer:
bool set_String(char *key_name, String *val);
```

#### Variable: time
```cpp
//unsigned long UTC epoch in seconds
unsigned long get_time(char *key_name);
bool set_time(char *key_name, unsigned long val);
```


#### Variable: hex
```cpp
byte *get_hex(char *key_name); //returns the pointer of the array

bool set_hex(char *key_name, byte val[], uint16_t len);
```


#### Variable: geo-coordinates (lat, long)
```cpp
bool set_geo(char *key_name, String val_lat, String val_lon);

String get_geo_lat(char *key_name);
String get_geo_lon(char *key_name);
```

#### Clear

```cpp
void ClearAllVariables();
void Clear(char *key_name); //clear a specific variable from memory
```


#### Verify

```cpp
bool isValidType(char *key_name, char type); //verify if the variable of this name and type exists
bool isValid(char *key_name); //verify if variable exists
```



### Data functions


```cpp
void DataClear(); //clear if there is some data in memory. It's cleared after Commit anyway.
void newDataRow(void);  //start a new data row with a timestamp
bool push_float(char *data_heading, double value);
bool push_long(char *data_heading, long value);
bool push_ulong(char *data_heading, unsigned long value);
bool push_int(char *data_heading, int value);
bool push_String(char *data_heading, String value);

int CommitData(TCPClientObj *TCPclient);
int SendJSON(TCPClientObj *TCPclient, String data_json);

//must use with the class prefix as Console.push_int('a', 1)
```


### Time functions

```cpp
// Real time can be parsed if the runSync() has been called at least once since restart
unsigned long realtime(); //UTC epoch in seconds
uint8_t weekday(void); //0=sunday,1=monday, 6=saturday
uint8_t year(void);	//021 for year 2021, 121 for year 2121
uint8_t month(void); //1-12
uint8_t date(void); //1-31
uint8_t hour(void); //0-24, UTC (set Timezone from console)
uint8_t minute(void); ///0-59

//must use with the class prefix as Console.weekday()

```

## Error codes
```cpp
ERR_FAILED_CONN -1      //Connection failed to the server
ERR_FAILED_CONN_10x -10 //10 times consistent failure. Probably no internet connection.
ERR_NO_NODE_TOKEN -2    //NT isn't identified, call Console.Identify(&TCPclient, mac_address); for a new NT.
ERR_ACK_FAILED -3       //usually due to wrong account security details. UDP Console.log() might still work because that's not encrypted.
ERR_DATA_PARSE -4       //usually due to wrong TTC or JSON syntax, try removing quotations.
```