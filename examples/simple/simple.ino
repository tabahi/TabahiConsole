//Last update: 14-07-2024, tested on ESP32Cam
#include <TabahiConsole.h>

const char* ssid     = "WiFi_Name";//e.g., "WiFi_Name";
const char* password = "WiFi_Pass";//e.g., "WiFi_Pass";

//Go to https://console.tabahi.tech/#account for the following settings:
#define TTC_server "api.tabahi.tech" //api.tabahi.tech
#define USER_TOKEN "6225868df6412032c74a3698" //e.g., 6225868df6412032c74a3698
#define USER_SECRET "Deu9DqvSS6pbNuIoI43aCh"  //e.g., Deu9DqvSS6pbNuIoI43aCh
#define DEBUG_TTC 1 //set to 1 to print verbose info on Serial

TTC Console(TTC_server, 2096, 44561, USER_TOKEN, USER_SECRET, DEBUG_TTC); //cloud server address, TCP channel port, UDP channel port, USER_TOKEN, USER_SECRET, enable_debug

String mac_address = ""; //will set in setup. Used for node identification.
int increment = 0; //dummy variable
unsigned long heartbeat = 10000; //delay between syncs

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
    // First, automatically identify (or create a new node_token) using mac_address. 
    // You can skip this part by
    // set_NODE_TOKEN("00000000000000000000000"); 
    // Go to console.tabahi.tech  > View or Add a node > Click view any node unit > (Gear button) Configure > Node Token
    if (Console.node_token_valid == false) //Need to get a Node Token before anything else. Console server keeps track of each node using a node_token that needs to be assigned first.
    {
      WiFiClient TCPclient;

      //Identify Node Token using mac address
      int idn_status = Console.Identify(&TCPclient, mac_address); 

      if (idn_status == 1) {
        Serial.print("Got NT: ");
        Serial.println(Console.NT);
      }
      else {
        Serial.printf("Identification failed %d\n", idn_status); //check your USER_TOKEN or USER_SECRET 
      }
    }

    if (Console.node_token_valid == true) //if already have a registered NT
    {
      sync_variables(); //sync, update, add, edit variables, see below

      push_data();    //push a data row to tables, see below


      //print some logs on UDP monitor:
      Console.log("UTC time: "); //will show on UDP monitor on console.tabahi.tech
      Console.logln(Console.realtime()); //time is synced with UTC epoch in total seconds (unsigned long)
      Console.logln("Done");
      Console.CommitLogs(mac_address.c_str()); //send all the monitor logs to server, using mac address as the identifier
      //must call CommitLogs() after log() to send logs to the server.
    }

    //check if a there is a variable 'example' on the console
    if (Console.isValidType("example", 'b'))
      bool example = Console.get_bool("example"); // this variable was set on the console and read here.
    // now you can locally use `example' to turn on or off a light switch


     //setting a variable to a new value
    increment++;
    Console.set_int("increment", increment); //this new value will show on the console after the next sync cycle
  
  

    delay(heartbeat); //wait for heartbeat seconds
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
  WiFiClient TCPclient; //define a TCP clinet to be used by Console
  int n_vars = Console.runSync(&TCPclient);

  if (n_vars >= 0) //check if variables sync was ok
  {
    Console.logln("Sync: OK");
    Console.printVariables(); //print all the vairables on Serial
  }
  else
    Serial.printf("Sync failed, status: %d \n", n_vars);

  unsigned long heartbeat = Console.get_ulong("heartbeat"); // delay between syncs
  
  return n_vars;  //returns number of synced variables. Error if it's negative.

  /* ERRORS returned as n_vars:
  ERR_FAILED_CONN -1      //Connection failed to the server
  ERR_FAILED_CONN_10x -10 //10 times consistent failure. Probably no internet connection.
  ERR_NO_NODE_TOKEN -2    //NT isn't identified, call Console.Identify(&TCPclient, mac_address); for a new NT.
  ERR_ACK_FAILED -3       //usually due to wrong account security details. UDP Console.log() might still work because that's not encrypted.
  ERR_DATA_PARSE -4       //usually due to wrong TTC or JSON syntax, try removing quotations.
  Otherwise, >=0  represents the successful sync of n number of variables.
  */
}










void push_data()
{
  //create a data row:
  Console.newDataRow(); //new row with a current timestamp
  Console.push_ulong("a",  123);
  Console.push_float("b", 456.789);
  Console.push_String("geo_loc", "1.223, -5.235"); //"using "geo" in the name will link to google maps

  WiFiClient TCPclient;
  //Data is not sent until 'CommitData' is called.
  if (Console.CommitData(&TCPclient) > 0) Console.logln("Data sent");

}
