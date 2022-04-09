

//Errors:
#define ERR_FAILED_CONN -1
#define ERR_FAILED_CONN_10x -10
#define ERR_NO_NODE_TOKEN -2
#define ERR_ACK_FAILED -3
#define ERR_DATA_PARSE -4

#define TOKEN_LENGTH 24 //it's always 24 characters
#define USER_SEC_LENGTH 22 //it's always less than 20

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

//checking if compiling for compatible boards
#if defined(ESP8266)
#define BOARD_ESP 8266
#elif defined(ESP32)
#define BOARD_ESP 32
#elif defined(__AVR__)
#define BOARD_ESP 0
#error Architecture is AVR instead of ESP8266 OR ESP32
#else
#define BOARD_ESP 0
#error Architecture is NOT ESP8266 OR ESP32
#endif

//Importing WiFi Client libraries
#if BOARD_ESP == 8266
#include <ESP8266WiFi.h> //required for importing WiFiClient and WiFiUDP, can use other TCP/UDP clients if available
#elif BOARD_ESP == 32
#include <WiFi.h> //required for importing WiFiClient and WiFiUDP, can use other TCP/UDP clients if available
#endif

#include <WiFiUdp.h>
#include <SettingsTTC.h>


//weather classes:
#if defined(WEATHER_HOURS_MAX) && (WEATHER_HOURS_MAX > 0)

struct SymbolClass
{
	const char *symbol;
	uint8_t snow_index;
	uint8_t rain_index;
	uint8_t fog_index;
};

struct ForecastClass
{
	float temp;
	float humid;
	uint8_t perc; //precipitation 0 to 1 value
	uint8_t snow; //snow symbols -> probability
	uint8_t rain; //rain symbols -> probability
	uint8_t fog;  //fog symbols -> probability
};

#define INVALID_VALUE -127
#define TOTAL_SYMBOLS 41

#endif

//Data class
struct DataClass
{
	char head[MAX_DATA_HEAD_LEN];
	char value[MAX_DATA_VALUE_LEN];
	uint8_t batch = 0;
};


//Variable class
struct VarsClass
{
	char type;
	char keyword[MAX_NAME_LEN + 1];
	String value;
};

class TTC
{
	const char *_TTC_server;
	int TT_TCP_port = 2096;
	int TT_UDP_port = 44561;
	const char *_USER_TOK;
	const char *_USER_SEC;

public:
	TTC(const char *TTC_server, int TT_TCP_portx, int TT_UDP_portx, const char *USER_TOK, const char *USER_SEC, const bool en_print_logs);
	bool _print_logs = false;
	bool node_token_valid = false;
	char NT[TOKEN_LENGTH] = "00000000000000000000000";

	unsigned long TTC_connection_timeout = 20000;
	uint8_t inbox = 0;
	bool first_sync_complete = 0;
	struct VarsClass vars[MAX_VARS] = {
		{'u', RUN_MINUTES, "0"}};

	struct DataClass DataBuff[MAX_DATA_ENTRIES];


	void initialize(void);
	void initialize(const char *TTC_server, int TT_TCP_portx, int TT_UDP_portx, const char *USER_TOK, const char *USER_SEC, bool en_print_logs);
	bool set_NODE_TOKEN(char *new_NT);
	String jsonify_var(uint8_t);
	byte *get_hex(char *key_name);
	bool set_hex(char *key_name, byte val[], uint16_t len);
	float get_float(char *key_name);
	bool set_float(char *key_name, float val);
	long get_long(char *key_name);
	bool set_long(char *key_name, long val);
	unsigned long get_ulong(char *key_name);
	bool set_ulong(char *key_name, unsigned long val);
	int16_t get_int(char *key_name);
	bool set_int(char *key_name, int16_t val);
	bool get_bool(char *key_name);
	bool set_bool(char *key_name, bool val);
	unsigned long get_time(char *key_name);
	bool set_time(char *key_name, unsigned long val);
	String get_String(char *key_name);
	String get(char *key_name);
	bool set_String(char *key_name, String *val);
	bool set_String(char *key_name, String val);
	bool set_geo(char *key_name, String val_lat, String val_lon);
	String get_geo_lat(char *key_name);
	String get_geo_lon(char *key_name);
	bool set(char *key_name, String *val);
	bool set(char *key_name, String val);
	bool set(char *key_name, bool val);
	bool set(char *key_name, int16_t val);
	bool set(char *key_name, long val);
	bool set(char *key_name, unsigned long val);
	bool set(char *key_name, float val);

	void ClearAllVariables();
	void Clear(char *key_name);
	bool isValidType(char *key_name, char type);
	bool isValid(char *key_name);
	void printVariables();

	int Identify(TCPClientObj *TCPclient, String mac_id_str);
	int runSync(TCPClientObj *TCPclient);
	//int runSync(TCPClientObj *TCPclient, bool send_variables);
	int runSync(TCPClientObj *TCPclient, String script_token, String args_text);
	int sendMessage(TCPClientObj *TCPclient, String to, String msg_text);
	String readMessage(TCPClientObj *TCPclient, char read_order);

	void DataClear();
	void newDataRow(void);
	bool push_float(char *data_heading, double value);
	bool push_long(char *data_heading, long value);
	bool push_ulong(char *data_heading, unsigned long value);
	bool push_int(char *data_heading, int value);
	bool push_String(char *data_heading, String value);
	bool push(char *data_heading, String value);
	bool push(char *data_heading, int value);
	bool push(char *data_heading, long value);
	bool push(char *data_heading, unsigned long value);
	bool push(char *data_heading, double value);
	int CommitData(TCPClientObj *TCPclient);
	int SendJSON(TCPClientObj *TCPclient, String data_json);

	//void run_IO_cmd(String *cmdtxt);

	void cryptSetKey(const char *secret);
	unsigned long realtime(); //UTC epoch in seconds
	uint8_t weekday(void); //0=sunday,1=monday, 6=saturday
	uint8_t year(void);	//021 for year 2021, 121 for year 2121
	uint8_t month(void); //1-12
	uint8_t date(void); //1-31
	uint8_t hour(void); //0-24, UTC (set Timezone from console)
	uint8_t minute(void); ///0-59

	void CommitLogs(const char *);
	void logln(int num);
	void log(int num);
	void logln(char c);
	void log(char c);
	void logln(long num);
	void log(long num);
	void logln(unsigned long num);
	void log(unsigned long num);
	void logln(double num);
	void log(double num);
	void logln(String str);
	void log(String str);
	void logln(const char *str);
	void log(const char *str);

	String fetchUpdateURL(TCPClientObj *TCPclient, const char *TTC_update_server, String use_idn);
	#if defined(ENABLE_OTA_UPDATE) && (ENABLE_OTA_UPDATE == 1)
	bool executeOTAupdate(String bin_link);
	String getHeaderValue(String header, String headerName);
	#endif

#if defined(WEATHER_HOURS_MAX) && (WEATHER_HOURS_MAX > 0)
	void weather_init(void);
	int fetchWeather(TCPClientObj *TCPclient, String geo_lat, String geo_lon, int forecast_hours);

	struct ForecastClass forecast[WEATHER_HOURS_MAX];
#endif

private:
	UCPClientObj Udp;
	bool RTC_synced = 0;
	unsigned long synced_time = 0;
	unsigned long synced_millis = 0;
	uint8_t time_parts[6] = {0,0,0,0,0,0}; //weekday, year, month, day, hour, minutes
	
	void Start_AUTH(TCPClientObj *TCPclient_x);
	void time_drift_fix(void);
	String jsonify_data(uint16_t data_num);
	bool hasData(void);
	bool DataIsValid(uint16_t data_num);
	uint8_t DataBatchNo(uint16_t data_num);
	bool DataFull(void);
	bool push_numeric_buffer(char *data_heading, char *buff);
	bool confirm_ascii_numeric(char this_n);
	bool confirm_ascii(char this_char);

	void TCP_send_enc(TCPClientObj *TCPclient_x, String to_send); //encrypts a String while sending over TCP
	char json_buffer_read_char(void);
	bool json_buffer_available(void);
	void json_buffer_start(char);
	void vars_char_parse(char);
	uint8_t vars_parse_count(void);
	void vars_parse_start(void);
	bool set_var(char type, String *keyword, String *value);
	uint16_t blank_var_i(void);
	void bytes_to_hex_string(byte val[], uint16_t len, char *buffer);


	char *byte2hex(byte val);						 //not used
	byte *hex2bytes(char *hex_in, uint16_t hex_len); //not used
	char dec_char(byte c);
	byte enc_char(char c);
	byte *enc_ctring(char *sc);
	void dec_start();
	void enc_start();


	// Internal variables

	uint16_t data_i = 0;
	uint8_t current_batch = 0;

	String logs_str = "";
	bool print_logs = true;
	uint16_t TCP_fails = 0;
	uint8_t failed_data_ack = 0;

	uint16_t rep_init_n = 0;
	uint8_t reply_flag = 0;
	char reply_type = '\0';
	String reply_accum = "";
	uint8_t rep_i = 0;
	uint8_t vars_s = 0;
	uint8_t vars_s_len = 0;
	bool var_first = 0;

	String reply_io = "";

	uint8_t pflag = 0;
	uint8_t parsed_count = 0;
	char cmdtype = '0';
	String p_accu = "";
	uint8_t p_accu_n = 0;
	uint8_t p_comma_n = 0;

	String vars_parse_head = "";
	char vars_parse_type = '\0';

	uint8_t batch_s = 0;

	//encryption:
	uint8_t enc_c = 0;
	uint8_t enc_i = 0;
	uint8_t enc_last = 99;
	uint8_t enc_sum = 0;
	uint8_t dec_c = 0;
	uint8_t dec_i = 0;
	uint8_t dec_last = 99;
	uint8_t dec_sum = 0;
	uint8_t key_len = 0;
	byte *sec_key = NULL;
	const uint8_t cip[8] = {211, 146, 235, 31, 245, 93, 129, 23}; //internal cipher pin, <255

#if defined(WEATHER_HOURS_MAX) && (WEATHER_HOURS_MAX > 0)
	void weather_parsing_reset(void);
	void parse_weather(char c);
	void match_weather_severity(void);

	uint8_t w_flag = 0;
	uint8_t w_count = 0;
	uint8_t w_comma = 0;
	String w_accum = "";
	int p_hr = INVALID_VALUE;
	float p_temp = INVALID_VALUE;
	float p_humd = INVALID_VALUE;
	float p_perp = INVALID_VALUE;	//percipitation
	int16_t p_snow = INVALID_VALUE; //snow symbols
	int16_t p_rain = INVALID_VALUE; //rain symbols
	int16_t p_fog = INVALID_VALUE;	//fog symbols
#endif
};
