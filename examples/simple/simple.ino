
#include <TabahiConsole.h>

const char* ssid     = "WiFi Name";
const char* password = "WiFi Pass";

#define TTC_server "api.tabahi.tech" //api.tabahi.tech
#define USER_TOKEN "6225868df6412032c74a3698"
#define USER_SECRET "E4aGDqvSS6pbNuIoInnBEh"
#define DEBUG_TTC 1 //set to 1 to print verbose info on Serial

TTC Console(TTC_server, 2096, 44561, USER_TOKEN, USER_SECRET, DEBUG_TTC);

String mac_address = ""; //will set in setup. Used for node identification.
int increment = 0; //dummy variable


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

      //Identify Node Token using mac address
      int idn_status = Console.Identify(&TCPclient, mac_address);

      if (idn_status == 1) {
        Serial.print("Got NT: ");
        Serial.println(Console.NT);
      }
      else {
        Serial.printf("Identification failed %d\n", idn_status);
      }
    }

    if (Console.node_token_valid == true) //if have a registered NT
    {
      sync_variables(); //sync, update, add, edit variables

      push_data();    //push data to tables


      //print some logs on UDP monitor:
      Console.log("UTC time: "); //will show on UDP monitor on console.tabahi.tech
      Console.logln(Console.realtime()); //time is synced with UTC epoch in total seconds (unsigned long)
      Console.logln("Done");
      Console.CommitLogs(mac_address.c_str()); //send all the monitor logs to server, using mac address as the identifier
      //must call CommitLogs() after log() to send logs to the server.
    }

    delay(30000); //wait for 30 seconds
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

  unsigned long heartbeat = Console.get_ulong("heartbeat");

  //check if a there is a variable 'example'
  if (Console.isValidType("example", 'b'))
    bool example = Console.get_bool("example");

  //setting a variable to a new value
  increment++;
  Console.set_int("increment", increment); //this new value will show on the console after the next sync cycle

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
