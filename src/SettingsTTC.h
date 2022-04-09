

#define WEATHER_HOURS_MAX 24 //Set to 0 to disable weather forecast. Set number of hours for which to fetch weather forecast
#define ENABLE_OTA_UPDATE 1 //set to 0 to disable function executeOTAupdate()....  fetchUpdateURL() is always included

#define TTC_INTERNAL_LOGS 1 //Set to 0 to disable logs from the internal functions of this library, Set to 1 to see debugging info

//For variables:
#define MAX_VARS 60 //maximum number of variables to hold
#define MAX_NAME_LEN 20
#define MAX_VALUE_LEN 50

// For data:
#define MAX_DATA_ENTRIES 60   //size of the buffer to hold data till successfull commit
#define MAX_DATA_HEAD_LEN 9   //max size of heading. 9 characters are enough
#define MAX_DATA_VALUE_LEN 18 //max size of value. Can increase if numbers are too big and JSON parsing is giving error


//variable names:
#define RUN_MINUTES "runMins" //minutes since power on



#define TCPClientObj WiFiClient //can change to other clients if available
#define UCPClientObj WiFiUDP

