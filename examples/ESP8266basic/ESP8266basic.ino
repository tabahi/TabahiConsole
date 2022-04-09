//version: 2021-09-23
/*
   Web:  https://tabahi.tech/
   1 - Set your WiFi SSID and password
   2 - Set your USER_TOKEN and USER_SECRET. Copy from 'account' page on mettabahi.tech
   3 - Create some variables using set_int("var_a", 123); or set_float("var_b", 123.65);
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
#define USER_TOKEN "61554cc872acbfeaa98282b8"
#define USER_SECRET "749b098fbd3f45"
TTC Console(TTC_server, 2096, 44561, USER_TOKEN, USER_SECRET);


String mac_id = "0000"; //will set in setup
long heartbeat = 30000;  //connection interval time
unsigned long conn_timer = 0;   //connection timer
boolean send_variables = false; //set true after recieving first load of variables from server
String node_token = "";

void setup()
{
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
  String version_string = "hello123";

  //Set initial values for variables:
  Console.set_String("version", version_string);
  Console.set_long("heartbeat", heartbeat);
  Console.set_int("int_var", 100);
  Console.set_float("float_var", 30.46);

  //hex is a little different
  byte hex_values[8] = {0xFF, 0x11, 0x22, 0x99, 0x00, 0xCC, 0xAA, 0xBB};
  Console.set_hex("hex_var", hex_values, 8);

  Console.printVariables(); //print on Serial

  Console.log(F("VER: ")); Console.logln(version_string); //log the version to UDP monitor

  delay(10);

  Console.logln(F("Ready\n\n"));
  Console.CommitLogs(mac_id); //send logs to server using mac_id as identifier
}



void loop()
{

  if ((millis() - conn_timer) > heartbeat)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      WiFiClient TCPclient;

      if (node_token.length() == 24)
      {
        int n_vars = Console.runSync(&TCPclient, node_token, send_variables);
        //int n_vars = Console.runSync(&TCPclient, node_token, send_variables, "script_token"); //use this example to run a script after each sync

        if (n_vars != -1) //check if variables sync request was successful
        {
          if (Console.isValidType("heartbeat", 'l')) //check if a valid long 'heartbeat' is synced
            heartbeat = Console.get_long("heartbeat");  //load the latest value recieved from server


          if (!send_variables) send_variables = true; // This is set 'true' after first sync to avoid sending initial values


          delay(1); yield();

          //send a data row:
          Console.newDataRow();
          Console.push_int("a", 10);
          Console.push_float("b", 10.6);
          Console.push_long("c", 9999999);
          Console.push_String("d", "yooooooo");
          if (Console.CommitData(&TCPclient, node_token)) Serial.println("Data sent");

          Console.log(F("Heap:")); Console.logln((long)ESP.getFreeHeap()); //check heap health

          verify_variables();  //see this function below
          show_synced_variables();  //see this function below
        }
      }
      else
      {
        if (Console.Identify(&TCPclient, mac_id) != -1) //get node token using the mac address as identifier
          node_token = Console.get_String(NODE_TOKEN);
      }

      delay(1); yield();

      Console.log("HB:");
      Console.logln(heartbeat);
      Console.CommitLogs(mac_id); //view logs in UDP monitor
    }
    else
    {
      Serial.println("Wifi not connected");
    }

    conn_timer = millis();
  }


  delay(0);
}






void show_synced_variables()
{
  Serial.print("version");    Serial.print('\t'); Serial.println(Console.get_String("version"));
  Serial.print("heartbeat");  Serial.print('\t'); Serial.println(Console.get_long("heartbeat"));
  Serial.print("int_var");    Serial.print('\t'); Serial.println(Console.get_int("int_var"));
  Serial.print("float_var");  Serial.print('\t'); Serial.println(Console.get_float("float_var"));

  if (Console.isValidType("hex_var", 'h'))
  {
    byte * hex_value = Console.get_hex("hex_var");
    Serial.print("hex_var\t");
    for (int b = 0; b < 8; b++) Serial.print(hex_value[b], HEX);
    Serial.println();
  }
}

void verify_variables()
{
  if (!Console.isValidType("hex_var", 'h'))
  {
    Serial.println("Invalid hex");
  }
  if (!Console.isValidType("version", 's'))
  {
    Serial.println("Invalid String");
  }
  if (!Console.isValidType("heartbeat", 'l'))
  {
    Serial.println("Invalid long");
  }
  if (!Console.isValidType("int_var", 'i'))
  {
    Serial.println("Invalid integer");
  }
  if (!Console.isValidType("float_var", 'f'))
  {
    Serial.println("Invalid float");
  }

}