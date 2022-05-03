
#include <ESPWifiConfig.h>
#include <TabahiConsole.h>

const char* ssid     = "SSE Broadband D1CC3D";
const char* password = "7JnaAdaGNFsErnxY";

#define TTC_server "api.tabahi.tech" //api.tabahi.tech
#define USER_TOKEN "6225868df6412032c74a3698"
#define USER_SECRET "E4aGDqvSS6pbNuIoInnBEh"
#define DEBUG_TTC 1 //set to 1 to print verbose info on Serial

TTC Console(TTC_server, 2096, 44561, USER_TOKEN, USER_SECRET, DEBUG_TTC);

String mac_address = ""; //will set in setup. Used for node identification.
unsigned long heartbeat = 10000;

String node_token = ""; //hold the node_token string character

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

  //Console.ClearAllVariables();  //clears synced variables - No need at the start

  //Console.set_NODE_TOKEN("61800000000000000000000"); //if NODE_TOKEN (NT) is already known then 'Console.Identify()' can be skipped. Use if (Console.node_token_valid == true)

  delay(5000); //wait 5 sec for Wifi to connect
}


float geo_lat = 50.9081;
float geo_lon = -0.1256;
int increment = 1;

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {

    WiFiClient TCPclient;

    if (Console.node_token_valid == false) //Need to get a Node Token before anything else
    {
      int idn_status = Console.Identify(&TCPclient, mac_address);  //Identify Node Token using mac address
      if (idn_status == 1)
      {
        Serial.print("NT: "); Serial.println(Console.NT);
        //Warn: DON'T SEND NT OVER UDP (INSECURE). CommitLogs sends logs over UDP without encryption. All other functions use TCP with encryption using USER_SECRET as the key.
      }
      else
      {
        Serial.printf("IDN failed %d\n", idn_status);
      }
    }

    else
    {
      checkSunrise();
      checkWeatherForecast(3);
      int n_vars = Console.runSync(&TCPclient);
      //

      if (n_vars >= 0) //check if variables sync was ok
      {
        Serial.printf("Inbox: %d \n", Console.inbox);

        //>12,624e767c7a9f1e8c3b8647ec,Hello hi
        //>seconds_ago,sender_node_token,Msg_content

        while (Console.inbox > 0)
        {
          String readMsg = Console.readMessage(&TCPclient, 't');
          if (readMsg[0] == '>')
          {
            int comma_index = readMsg.indexOf(',');
            unsigned long seconds_ago = atol(readMsg.substring(1, comma_index).c_str());
            readMsg = readMsg.substring(comma_index + 1); comma_index = readMsg.indexOf(',');
            String sender_NT = readMsg.substring(0, comma_index);
            String message = readMsg.substring(comma_index + 1);

            Serial.print("Recieved: "); Serial.print(seconds_ago); Serial.print(" seconds ago, from "); Serial.println(sender_NT);
            Serial.print("Msg:"); Serial.println(message);
          }
          else
          {
            Serial.print("Error:"); Serial.println(readMsg);
          }
          Console.inbox--;
        }

        //Send message to self (@mac_address, use Node Token Console.NT or identifier)
        int sendMsgStatus = Console.sendMessage(&TCPclient, mac_address, "yo, sup");
        Serial.print("Msg sent:\t"); Serial.println(sendMsgStatus);

        if (Console.isValidType("heartbeat", 'u')) //check if a valid long 'heartbeat' is synced
          heartbeat = Console.get_ulong("heartbeat");  //load the latest value recieved from server
        else Serial.println("No 'u' var for heartbeat");


        Console.printVariables();
        heartbeat = 10000;
        increment++; increment %= 4;

        Console.set_ulong("heartbeat", heartbeat);     //it's already on console, it will change it's value
        Console.set_int("increment", increment);       //it will create a new variable
        Console.set_time("testTime", Console.realtime() + 300); //set a time variable 300 seconds in the future (5 mins)
        Console.set_geo("geo1", "51.509", "-0.1256"); //set a Lat,long geo variable... Note: use Strings instead of floats

        if (Console.isValid("example")) //check if a valid long 'test_var' is synced
        {
          Serial.print("example=");
          Serial.println(Console.get_bool("example"));
        }

      }
      else
      {
        Serial.printf("Sync failed %d \n", n_vars);
      }

      Serial.print("UTC time: ");
      Serial.println(Console.realtime()); //time is synced with UTC epoch in total seconds (unsigned long)

      //Print time in read-able format:
      Serial.printf("Time:\t %d-%d-%d %d:%d \tweekday:%d\n", Console.year(), Console.month(), Console.date(), Console.hour(), Console.minute(), Console.weekday());

      delay(1); yield();  //a minor delay for wifi stability

      //create a data row:
      Console.newDataRow(); //new row with a current timestamp
      Console.push_ulong("a",  123);
      Console.push_float("b", 456.789);
      geo_lat += 0.01;
      geo_lon -= 0.01;
      Console.push_String("c_geo", String(geo_lat) + "," + String(geo_lon));  //heading including 'geo' will link to google maps
      Console.push("d", "test_ok");


      //Data is not sent until 'CommitData' is called.
      if (Console.CommitData(&TCPclient) > 0) Serial.println("Data sent");


      //Alternatively, you can also send Data in one go using JSON format as:
      Console.SendJSON(&TCPclient, "{\"row1\":{\"data1\": 90, \"data2\": 0.211, \"data3\": \"chill\"}, \"time\":" + String(Console.realtime()) + "}");
      Console.SendJSON(&TCPclient, "{\"row2\":{\"data1\": 80, \"data2\": 0.456, \"data3\": \"pill\"}}"); //server adds it's own timestamp
      //Don't make rows too big to handle for memory
      //No need to CommitData after SendJSON

      Console.log(F("Heap:")); Console.logln((long) ESP.getFreeHeap()); //check heap health

      Console.CommitLogs(mac_address.c_str());//send all the monitor logs to server, using mac address as the identifier

      //Important:
      //Sync and data functions use Node Token and communicate over secure TCP
      //log (Monitor) uses mac_address (does't need Node Token) and communictae over insecure UDP
      //UDP logging (Monitor) is more agile and less prone to failure.
    }

    Serial.printf("Wait for %d seconds\n", heartbeat / 1000);
    delay(heartbeat);

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


void checkWeatherForecast(int forecast_hours)
{
#if defined(WEATHER_HOURS_MAX) && (WEATHER_HOURS_MAX>0)
  //Should define at top of SettingsTTC.h :
  //#define WEATHER_HOURS_MAX 12
  //Then pass parameters as:
  String geo_lat = "60.362";
  String geo_lon = "5.013";
  //int forecast_hours = WEATHER_HOURS_MAX;

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

#endif
}
