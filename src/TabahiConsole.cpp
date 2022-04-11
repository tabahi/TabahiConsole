
#include "TabahiConsole.h"

TTC::TTC(const char *TTC_server, int TT_TCP_portx, int TT_UDP_portx, const char *USER_TOK, const char *USER_SEC, bool en_print_logs)
{
  _TTC_server = TTC_server;
  if (TT_TCP_portx > 0)
    TT_TCP_port = TT_TCP_portx;
  if (TT_UDP_portx > 0)
    TT_UDP_port = TT_UDP_portx;
  _USER_TOK = USER_TOK;
  _USER_SEC = USER_SEC;
  _print_logs = en_print_logs;
};

void TTC::initialize(const char *TTC_server, int TT_TCP_portx, int TT_UDP_portx, const char *USER_TOK, const char *USER_SEC, bool en_print_logs)
{
  _TTC_server = TTC_server;
  if (TT_TCP_portx > 0)
    TT_TCP_port = TT_TCP_portx;
  if (TT_UDP_portx > 0)
    TT_UDP_port = TT_UDP_portx;
  _USER_TOK = USER_TOK;
  _USER_SEC = USER_SEC;
  initialize();
}

void TTC::initialize(void)
{
  data_i = 0;
  batch_s = 0;
  newDataRow();

  logs_str.reserve(200);

  //first, keep a count of initalized vars in code
  for (int i = 0; i < MAX_VARS; i++)
  {
    if ((vars[i].type < 48) || (vars[i].type > 122))
      rep_init_n = i;
  }

  cryptSetKey(_USER_SEC);

#if defined(WEATHER_HOURS_MAX) && (WEATHER_HOURS_MAX > 0)
  weather_init();
#endif
}

bool TTC::hasData()
{
  if (data_i > 0)
    return 1;
  return 0;
}

bool TTC::DataIsValid(uint16_t data_num)
{
  if (DataBuff[data_num].head[0] != '\0')
    return 1;
  return 0;
}

uint8_t TTC::DataBatchNo(uint16_t data_num) //get Batch no of a data point
{
  return DataBuff[data_num].batch;
}

void TTC::newDataRow(void)
{
  //data separator
  //adds timestamp to the data that comes after it
  current_batch++;
  current_batch %= 26; //a to z
  push_long("t0", millis());
}

void TTC::DataClear(void)
{
  for (uint16_t i = 0; i < MAX_DATA_ENTRIES; i++)
  {
    DataBuff[i].head[0] = '\0';
  }
  data_i = 0;
}

bool TTC::push(char *data_heading, String value)
{
  return push_String(data_heading, value);
}

bool TTC::push(char *data_heading, int value)
{
  return push_int(data_heading, value);
}

bool TTC::push(char *data_heading, long value)
{
  return push_long(data_heading, value);
}

bool TTC::push(char *data_heading, unsigned long value)
{
  return push_ulong(data_heading, value);
}

bool TTC::push(char *data_heading, double value)
{
  return push_float(data_heading, value);
}

bool TTC::push_String(char *data_heading, String value)
{
  if (data_i >= MAX_DATA_ENTRIES)
    data_i = 0;

  for (uint8_t i = 0; i < MAX_DATA_HEAD_LEN; i++)
  {
    DataBuff[data_i].head[i] = data_heading[i];
    if (data_heading[i] == '\0')
      break;
  }
  DataBuff[data_i].value[0] = '"';
  uint8_t v_len = value.length();
  uint8_t i = 1;
  while ((i < MAX_DATA_VALUE_LEN - 1) && (i - 1 < v_len))
  {
    DataBuff[data_i].value[i] = value[i - 1];
    i++;
  }

  DataBuff[data_i].value[i] = '"';
  i++;
  while (i < MAX_DATA_VALUE_LEN)
  {
    DataBuff[data_i].value[i] = '\0';
    i++;
  }
  DataBuff[data_i].batch = current_batch;
  data_i++;
  if (data_i >= MAX_DATA_ENTRIES)
  {
#if TTC_INTERNAL_LOGS
    logln(F("[data] ERR: buff full"));
#endif
    return 0;
  }

  return 1;
}

bool TTC::push_int(char *data_heading, int value)
{
  if (data_i >= MAX_DATA_ENTRIES)
    data_i = 0;
  char buff[MAX_DATA_VALUE_LEN];
  itoa(value, buff, 10); //convert to char array

  return push_numeric_buffer(data_heading, buff); //returns false if buffer is full after this
}

bool TTC::push_float(char *data_heading, double value)
{
  if (data_i >= MAX_DATA_ENTRIES)
    data_i = 0;
  char buff[MAX_DATA_VALUE_LEN];
  dtostrf(value, MAX_DATA_VALUE_LEN - 3, 3, buff); //3rd arg is floating point precision

  return push_numeric_buffer(data_heading, buff); //returns false if buffer is full after this
}

bool TTC::push_long(char *data_heading, long value)
{
  char buff[MAX_DATA_VALUE_LEN];

  //convert long to char array:
  long lNo = value;
  int numbers[14], n = 0;
  bool negative = 0;
  if (value < 0)
  {
    negative = 1;
    lNo *= -1;
  }
  while (lNo > 0)
  {
    numbers[n] = lNo % 10;
    lNo = lNo / 10;
    n++;
  }

  for (uint8_t k = 0; k < MAX_DATA_VALUE_LEN; k++)
    buff[k] = '\0';
  int i = 0;
  if (negative)
  {
    buff[0] = '-';
    i++;
  }
  buff[n] = '\0';
  n--;
  while (n >= 0)
  {
    buff[i] = numbers[n] + '0';
    n--;
    i++;
  }
  return push_numeric_buffer(data_heading, buff); //returns false if buffer is full after this
}

bool TTC::push_ulong(char *data_heading, unsigned long value)
{
  char buff[MAX_DATA_VALUE_LEN];

  //convert long to char array:
  unsigned long lNo = value;
  int numbers[14], n = 0;
  while (lNo > 0)
  {
    numbers[n] = lNo % 10;
    lNo = lNo / 10;
    n++;
  }
  for (uint8_t k = 0; k < MAX_DATA_VALUE_LEN; k++)
    buff[k] = '\0';
  int i = 0;
  buff[n] = '\0';
  n--;
  while (n >= 0)
  {
    buff[i] = numbers[n] + '0';
    n--;
    i++;
  }
  return push_numeric_buffer(data_heading, buff); //returns false if buffer is full after this
}

bool TTC::push_numeric_buffer(char *data_heading, char *buff)
{
  if (data_i >= MAX_DATA_ENTRIES)
    data_i = 0;

  for (uint8_t i = 0; i < MAX_DATA_HEAD_LEN; i++)
  {
    DataBuff[data_i].head[i] = data_heading[i];
    if (data_heading[i] == '\0')
    {
      if (i == 0)
      {
        DataBuff[data_i].head[0] = 'N';
        DataBuff[data_i].head[1] = 'U';
        DataBuff[data_i].head[2] = 'L';
        DataBuff[data_i].head[3] = 'L';
        DataBuff[data_i].head[4] = '\0';
      }
      break;
    }
  }
  uint8_t non_blanks = 0;
  uint8_t i = 0;
  while (i < MAX_DATA_VALUE_LEN)
  {
    DataBuff[data_i].value[i] = *buff;
    *buff++;
    if (DataBuff[data_i].value[i] == '\0')
      break;
    else if (confirm_ascii_numeric(buff[i]))
      non_blanks++;
    i++;
  }
  while (i < MAX_DATA_VALUE_LEN)
  {
    DataBuff[data_i].value[i] = '\0';
    i++;
  }
  //Serial.print(DataBuff[data_i].head);
  //Serial.print('\t');
  //Serial.println(DataBuff[data_i].value);

  if (non_blanks == 0)
  {
    DataBuff[data_i].value[0] = '\"';
    DataBuff[data_i].value[1] = '\"';
    DataBuff[data_i].value[2] = '\0';
  }
  DataBuff[data_i].batch = current_batch;
  data_i++;
  if (data_i >= MAX_DATA_ENTRIES)
  {
#if TTC_INTERNAL_LOGS
    logln(F("[data] ERR: buff full"));
#endif
    return 0;
  }
  return 1;
}

bool TTC::DataFull(void)
{
  if (data_i >= MAX_DATA_ENTRIES)
    return 1;
  else
    return 0;
}

bool TTC::confirm_ascii_numeric(char this_n)
{
  //0-9, decimal and minus
  if (((this_n >= 48) && (this_n <= 57)) || (this_n == 46) || (this_n == 45))
    return 1;
  else
    return 0;
}

String TTC::jsonify_data(uint16_t data_num)
{
  String js = "";

  if (DataBuff[data_num].head[0] != '\0')
  {
    js = "\"";
    for (uint8_t i1 = 0; i1 < MAX_DATA_HEAD_LEN; i1++)
    {
      char c = DataBuff[data_num].head[i1];
      if (c == '\0')
        break;
      else
        js += c;
    }
    js += "\":";

    for (uint8_t i2 = 0; i2 < MAX_DATA_VALUE_LEN; i2++)
    {
      char c = DataBuff[data_num].value[i2];
      if (c == '\0')
        break;
      else
        js += c;
    }
  }
  //Serial.println(js);

  return js;
}

//variable fuctions:

void TTC::printVariables()
{
  for (int i = 0; i < MAX_VARS; i++)
  {
    if ((vars[i].type >= 48) && (vars[i].type <= 122))
    {
      Serial.print(vars[i].type);
      Serial.print('\t');
      Serial.print(vars[i].keyword);
      Serial.print('=');
      Serial.println(vars[i].value);
    }
  }
}

bool TTC::isValid(char *key_name)
{
  for (int i = 0; i < MAX_VARS; i++)
  {
    if ((vars[i].type >= 48) && (vars[i].type <= 122) && (strcmp(vars[i].keyword, key_name) == 0))
    {
      return 1;
    }
  }
  return 0;
}

bool TTC::isValidType(char *key_name, char type)
{
  for (int i = 0; i < MAX_VARS; i++)
  {
    if ((vars[i].type == type) && (strcmp(vars[i].keyword, key_name) == 0))
    {
      return 1;
    }
  }
  return 0;
}

void TTC::Clear(char *key_name)
{
  for (int i = 0; i < MAX_VARS; i++)
  {
    if ((vars[i].type >= 48) && (vars[i].type <= 122) && (strcmp(vars[i].keyword, key_name) == 0))
    {
#if TTC_INTERNAL_LOGS
      log("[vars] Cleared: ");
      logln(vars[i].keyword);
#endif
      vars[i].type = 0;
      vars[i].keyword[0] = NULL;
      vars[i].value = "";
      break;
    }
  }
}

void TTC::ClearAllVariables()
{
  for (int i = 0; i < MAX_VARS; i++)
  {
    if ((vars[i].type >= 48) && (vars[i].type <= 122))
    {
      vars[i].type = 0;
      vars[i].keyword[0] = NULL;
      vars[i].value = "";
    }
  }
  first_sync_complete = 0;
}

bool TTC::set(char *key_name, String *val)
{
  return set_String(key_name, val);
}

bool TTC::set(char *key_name, String val)
{
  return set_String(key_name, val);
}

bool TTC::set(char *key_name, bool val)
{
  return set_bool(key_name, val);
}

bool TTC::set(char *key_name, int16_t val)
{
  return set_int(key_name, val);
}

bool TTC::set(char *key_name, long val)
{
  return set_long(key_name, val);
}

bool TTC::set(char *key_name, unsigned long val)
{
  return set_ulong(key_name, val);
}

bool TTC::set(char *key_name, float val)
{
  return set_float(key_name, val);
}

bool TTC::set_String(char *key_name, String *val)
{
  uint16_t i = 0;
  while (i < MAX_VARS)
  {
    if ((vars[i].type == 's') && (strcmp(vars[i].keyword, key_name) == 0))
      break;
    else
      i++;
  }
  if (i == MAX_VARS)
    i = blank_var_i();

  if (i < MAX_VARS)
  {
    vars[i].type = 's';
    for (size_t c = 0; c <= MAX_NAME_LEN; ++c)
    {
      vars[i].keyword[c] = key_name[c];
      if (key_name[c] == '\0')
        break;
    }
    vars[i].value = (*val);
    return true;
  }
  else
    return false;
}

String TTC::get_String(char *key_name)
{
  for (int i = 0; i < MAX_VARS; i++)
  {
    if ((vars[i].type == 's') && (strcmp(vars[i].keyword, key_name) == 0))
      return vars[i].value;
  }
#if TTC_INTERNAL_LOGS
  log(F("[vars] ERR NO VAR(s):"));
  logln(key_name);
#endif
  return "";
}

bool TTC::set_geo(char *key_name, String val_lat, String val_lon)
{
  uint16_t i = 0;
  while (i < MAX_VARS)
  {
    if ((vars[i].type == 'g') && (strcmp(vars[i].keyword, key_name) == 0))
      break;
    else
      i++;
  }
  if (i == MAX_VARS)
    i = blank_var_i();

  if (i < MAX_VARS)
  {
    vars[i].type = 'g';
    for (size_t c = 0; c <= MAX_NAME_LEN; ++c)
    {
      vars[i].keyword[c] = key_name[c];
      if (key_name[c] == '\0')
        break;
    }
    vars[i].value = val_lat + "," + val_lon;
    return true;
  }
  else
    return false;
}

String TTC::get_geo_lat(char *key_name)
{
  for (int i = 0; i < MAX_VARS; i++)
  {
    if ((vars[i].type == 'g') && (strcmp(vars[i].keyword, key_name) == 0))
      return vars[i].value.substring(0, vars[i].value.indexOf(','));
  }
#if TTC_INTERNAL_LOGS
  log(F("[vars] ERR NO VAR(g):"));
  logln(key_name);
#endif
  return "";
}

String TTC::get_geo_lon(char *key_name)
{
  for (int i = 0; i < MAX_VARS; i++)
  {
    if ((vars[i].type == 'g') && (strcmp(vars[i].keyword, key_name) == 0))
      return vars[i].value.substring(vars[i].value.indexOf(',') + 1);
  }
#if TTC_INTERNAL_LOGS
  log(F("[vars] ERR NO VAR(g):"));
  logln(key_name);
#endif
  return "";
}

String TTC::get(char *key_name)
{
  for (int i = 0; i < MAX_VARS; i++)
  {
    if ((vars[i].type >= 'a') && (vars[i].type <= 'z') && (strcmp(vars[i].keyword, key_name) == 0))
      return vars[i].value;
  }
#if TTC_INTERNAL_LOGS
  log(F("[vars] ERR NO VAR(any):"));
  logln(key_name);
#endif
  return "";
}

bool TTC::set_bool(char *key_name, bool val)
{
  uint16_t i = 0;
  while (i < MAX_VARS)
  {
    if ((vars[i].type == 'b') && (strcmp(vars[i].keyword, key_name) == 0))
      break;
    else
      i++;
  }
  if (i == MAX_VARS)
    i = blank_var_i();

  if (i < MAX_VARS)
  {
    vars[i].type = 'b';
    for (size_t c = 0; c < 20; ++c)
    {
      vars[i].keyword[c] = key_name[c];
      if (key_name[c] == '\0')
        break;
    }
    vars[i].value = val ? "1" : "0";
    return true;
  }
  else
    return false;
}

bool TTC::get_bool(char *key_name)
{
  for (int i = 0; i < MAX_VARS; i++)
  {
    if ((vars[i].type == 'b') && (strcmp(vars[i].keyword, key_name) == 0))
      return vars[i].value[0] == '1' ? true : false;
  }

#if TTC_INTERNAL_LOGS
  log(F("[vars] ERR NO VAR(b):"));
  logln(key_name);
#endif
  return false;
}

bool TTC::set_int(char *key_name, int16_t val)
{
  uint16_t i = 0;
  while (i < MAX_VARS)
  {
    if ((vars[i].type == 'i') && (strcmp(vars[i].keyword, key_name) == 0))
      break;
    else
      i++;
  }
  if (i == MAX_VARS)
    i = blank_var_i();

  if (i < MAX_VARS)
  {
    vars[i].type = 'i';
    for (size_t c = 0; c <= MAX_NAME_LEN; ++c)
    {
      vars[i].keyword[c] = key_name[c];
      if (key_name[c] == '\0')
        break;
    }
    vars[i].value = String(val);
    return true;
  }
  else
    return false;
}

int16_t TTC::get_int(char *key_name)
{

  for (int i = 0; i < MAX_VARS; i++)
  {
    if ((vars[i].type == 'i') && (strcmp(vars[i].keyword, key_name) == 0))
      return vars[i].value.toInt();
  }
#if TTC_INTERNAL_LOGS
  log(F("[vars] ERR NO VAR(i):"));
  logln(key_name);
#endif
  return 0;
}

bool TTC::set_long(char *key_name, long val)
{
  uint16_t i = 0;
  while (i < MAX_VARS)
  {
    if ((vars[i].type == 'l') && (strcmp(vars[i].keyword, key_name) == 0))
      break;
    else
      i++;
  }
  if (i == MAX_VARS)
    i = blank_var_i();

  if (i < MAX_VARS)
  {
    vars[i].type = 'l';
    for (size_t c = 0; c <= MAX_NAME_LEN; ++c)
    {
      vars[i].keyword[c] = key_name[c];
      if (key_name[c] == '\0')
        break;
    }
    vars[i].value = String(val);
    return true;
  }
  else
    return false;
}

bool TTC::set_time(char *key_name, unsigned long val)
{
  uint16_t i = 0;
  while (i < MAX_VARS)
  {
    if ((vars[i].type == 't') && (strcmp(vars[i].keyword, key_name) == 0))
      break;
    else
      i++;
  }
  if (i == MAX_VARS)
    i = blank_var_i();

  if (i < MAX_VARS)
  {
    vars[i].type = 't';
    for (size_t c = 0; c <= MAX_NAME_LEN; ++c)
    {
      vars[i].keyword[c] = key_name[c];
      if (key_name[c] == '\0')
        break;
    }
    vars[i].value = String(val);
    return true;
  }
  else
    return false;
}

unsigned long TTC::get_time(char *key_name)
{
  for (int i = 0; i < MAX_VARS; i++)
  {
    if ((vars[i].type == 't') && (strcmp(vars[i].keyword, key_name) == 0))
      return strtoul(vars[i].value.c_str(), NULL, 10);
  }
#if TTC_INTERNAL_LOGS
  log(F("[vars] ERR NO VAR(t):"));
  logln(key_name);
#endif
  return 0;
}

bool TTC::set_ulong(char *key_name, unsigned long val)
{
  uint16_t i = 0;
  while (i < MAX_VARS)
  {
    if ((vars[i].type == 'u') && (strcmp(vars[i].keyword, key_name) == 0))
      break;
    else
      i++;
  }
  if (i == MAX_VARS)
    i = blank_var_i();

  if (i < MAX_VARS)
  {
    vars[i].type = 'u';
    for (size_t c = 0; c <= MAX_NAME_LEN; ++c)
    {
      vars[i].keyword[c] = key_name[c];
      if (key_name[c] == '\0')
        break;
    }
    vars[i].value = String(val);
    return true;
  }
  else
    return false;
}

unsigned long TTC::get_ulong(char *key_name)
{
  for (int i = 0; i < MAX_VARS; i++)
  {
    if ((vars[i].type == 'u') && (strcmp(vars[i].keyword, key_name) == 0))
    {
      return strtoul(vars[i].value.c_str(), NULL, 10);
    }
  }
#if TTC_INTERNAL_LOGS
  log(F("[vars] ERR NO VAR(u):"));
  logln(key_name);
#endif
  return 0;
}

long TTC::get_long(char *key_name)
{
  for (int i = 0; i < MAX_VARS; i++)
  {
    if ((vars[i].type == 'l') && (strcmp(vars[i].keyword, key_name) == 0))
      return atol(vars[i].value.c_str());
  }
#if TTC_INTERNAL_LOGS
  log(F("[vars] ERR NO VAR(l):"));
  logln(key_name);
#endif
  return 0;
}

bool TTC::set_float(char *key_name, float val)
{
  uint16_t i = 0;
  while (i < MAX_VARS)
  {
    if ((vars[i].type == 'f') && (strcmp(vars[i].keyword, key_name) == 0))
      break;
    else
      i++;
  }
  if (i == MAX_VARS)
    i = blank_var_i();

  if (i < MAX_VARS)
  {
    vars[i].type = 'f';
    for (size_t c = 0; c < 20; ++c)
    {
      vars[i].keyword[c] = key_name[c];
      if (key_name[c] == '\0')
        break;
    }
    vars[i].value = String(val);
    return true;
  }
  else
    return false;
}

float TTC::get_float(char *key_name)
{

  for (int i = 0; i < MAX_VARS; i++)
  {
    if ((vars[i].type == 'f') && (strcmp(vars[i].keyword, key_name) == 0))
      return vars[i].value.toFloat();
  }
#if TTC_INTERNAL_LOGS
  log(F("[vars] ERR NO VAR(f):"));
  logln(key_name);
#endif
  return 0;
}

bool TTC::set_hex(char *key_name, byte val[], uint16_t len)
{
  uint16_t i = 0;
  while (i < MAX_VARS)
  {
    if ((vars[i].type == 'h') && (strcmp(vars[i].keyword, key_name) == 0))
      break;
    else
      i++;
  }
  if (i == MAX_VARS)
    i = blank_var_i();

  if (i < MAX_VARS)
  {
    vars[i].type = 'h';
    char str[len * 2];
    bytes_to_hex_string(val, len, str);
    for (size_t c = 0; c < 20; ++c)
    {
      vars[i].keyword[c] = key_name[c];
      if (key_name[c] == '\0')
        break;
    }
    vars[i].value = String(str);
    return true;
  }
  else
    return false;
}

void TTC::bytes_to_hex_string(byte val[], uint16_t len, char *buffer) //binary(1 byte/byte) to Hex String (2 chars/byte)
{
  for (unsigned int i = 0; i < len; i++)
  {
    byte nib1 = (val[i] >> 4) & 0x0F;
    byte nib2 = (val[i] >> 0) & 0x0F;
    buffer[i * 2 + 0] = nib1 < 0xA ? '0' + nib1 : 'A' + nib1 - 0xA;
    buffer[i * 2 + 1] = nib2 < 0xA ? '0' + nib2 : 'A' + nib2 - 0xA;
  }
  buffer[len * 2] = '\0';
}

byte *TTC::get_hex(char *key_name) //convert String hex=FF to byte hex 0xFF
{
  byte *ret_arr = NULL;
  for (int i = 0; i < MAX_VARS; i++)
  {
    if ((vars[i].type == 'h') && (strcmp(vars[i].keyword, key_name) == 0))
    {
      uint16_t len = vars[i].value.length();
      uint16_t bn = len / 2;

      ret_arr = (byte *)malloc(bn * sizeof(byte)); // try to allocate memory

      while (len >= 2)
      {
        bn -= 1;
        char c2[2];
        c2[0] = vars[i].value[len - 2];
        c2[1] = vars[i].value[len - 1];
        ret_arr[bn] = strtoul(c2, NULL, 16);
        //ret[bn] = strtoul("E8", NULL, 16);
        len -= 2;
      }
      //byte ret[]= {0x28, 0xFF, 0xC7, 0xBA, 0x42, 0x16, 0x04, 0xE8 };
      return ret_arr;
    }
  }
#if TTC_INTERNAL_LOGS
  log(F("[vars] HEX not in mem:"));
  logln(key_name);
#endif

  return ret_arr;
}

uint16_t TTC::blank_var_i(void)
{
  for (int i = 0; i < MAX_VARS; i++)
  {
    if ((vars[i].type < 48) || (vars[i].type > 122))
    {
      return i;
    }
  }
  return MAX_VARS;
}

bool TTC::set_var(char type, String *keyword, String *value)
{
  uint8_t key_len = keyword->length() + 1;
  char key_name[MAX_NAME_LEN];
  keyword->toCharArray(key_name, key_len);

  bool in_mem = false;
  for (int i = 0; i < MAX_VARS; i++)
  {
    if (strcmp(vars[i].keyword, key_name) == 0)
    {
      if (vars[i].type != type)
        vars[i].type = type;
      vars[i].value = *value;
      in_mem = true;
      break;
    }
  }
  if (!in_mem)
  {

    uint16_t i = blank_var_i();
    if (i < MAX_VARS)
    {

      vars[i].type = type;
      for (size_t c = 0; c <= MAX_NAME_LEN; ++c)
      {
        vars[i].keyword[c] = key_name[c];
        if (key_name[c] == '\0')
          break;
      }
      vars[i].value = *value;
#if TTC_INTERNAL_LOGS
      log("[vars] New:\t");
      log(vars[i].type);
      log('\t');
      log(vars[i].keyword);
      log('\t');
      logln(vars[i].value);
#endif
      in_mem = true;
    }
#if TTC_INTERNAL_LOGS
    else
      logln("[vars] ERR: VARS_MAX full");
#endif
  }
  return in_mem;
}

//understand whatever received from server, and prepare reply buffers for data that is sent to server
/*
  //Format to set variable:
  $>typechar,name,value;
  $>i,abc,124;
  $>h,HEX_VAR,FFAABB1122334466;
  $>s,NODE_TOKEN,61217cdeca7cdc015a8b0212;

  Format for command
  #V to send vars, #S to sleep, #D for data push

  IO format
  _PA2=245; //under contruction

*/
void TTC::vars_parse_start()
{
  pflag = 0;
  parsed_count = 0;
}

uint8_t TTC::vars_parse_count()
{
  return parsed_count;
}

unsigned long TTC::realtime() //returns UTC epoch in seconds sine 1970
{
  //https://www.epochconverter.com/
  return synced_time + ((millis() - synced_millis) / 1000);
}

enum TimeEnum
{
  _weekday_k,
  _year_k,
  _month_k,
  _date_k,
  _hour_k,
  _minutes_k
};

void TTC::time_drift_fix(void)
{
  unsigned long m_now = millis();
  if ((m_now - synced_millis) < 60000)
    return;
	
  unsigned long smills = synced_millis;
  while ((m_now - smills) >= 60000)
  {
    time_parts[_minutes_k] += 1;
    smills += 60000;
  }

  while (time_parts[_minutes_k] >= 60)
  {
    time_parts[_hour_k] += 1;
    time_parts[_minutes_k] -= 60;
  }

  while (time_parts[_hour_k] >= 24)
  {
    time_parts[_date_k] += 1;
    time_parts[_hour_k] -= 24;
    time_parts[_weekday_k] += 1;
    if (time_parts[_weekday_k] > 6)
      time_parts[_weekday_k] = 0;
  }

  if (time_parts[_date_k] > 28)
    if ((time_parts[_date_k] > 31) || ((time_parts[_date_k] > 30) && (time_parts[_month_k] == 4 || time_parts[_month_k] == 6 || time_parts[_month_k] == 9 || time_parts[_month_k] == 11)) || ((time_parts[_month_k] == 2) && ((time_parts[_date_k] > 29) || (((time_parts[_year_k] % 4) == 0) && (time_parts[_date_k] > 28)))))
    {
      time_parts[_month_k] += 1;
      time_parts[_date_k] = 1;
    }

  if (time_parts[_month_k] > 12)
  {
    time_parts[_year_k] += 1;
    time_parts[_month_k] = 1;
  }
}

uint8_t TTC::weekday(void)
{
  time_drift_fix();
  return time_parts[_weekday_k];
}
uint8_t TTC::year(void)
{
  time_drift_fix();
  return time_parts[_year_k];
}
uint8_t TTC::month(void)
{
  time_drift_fix();
  return time_parts[_month_k];
}
uint8_t TTC::date(void)
{
  time_drift_fix();
  return time_parts[_date_k];
}
uint8_t TTC::hour(void)
{
  time_drift_fix();
  return time_parts[_hour_k];
}


uint8_t TTC::minute(void)
{
  time_drift_fix();
  return time_parts[_minutes_k];
}

void TTC::vars_char_parse(char c)
{
  //Format to set variable:
  //$>type char, name, value
  //$>i,a,443;

  //Format for command
  //#V to send vars, #S tp sleep, #D for data push

  //IO format
  //_PA2=245; //problem
  if (pflag == 0)
  {
    if (c == '$')
    {
      pflag = 1;
    }
    else if (c == '#')
      pflag = 3; //short command
    /*
      else if(c=='_') //Pin I/O read/set
      {
      p_accu = "";
      p_accu_n = 0;
      pflag = 4;
      }*/
  }
  else if (pflag == 3) ////cmd type: single char commands
  {
    //e.g., #~T for reset Node_Token  or     #~S
    if (c == '~')
      pflag = 3; //wait for next char
    else
    {
      json_buffer_start(c);
      pflag = 0;
    }
  } /*
  else if(pflag==4) ////cmd type: digital I/O commands
  {
    if((c==';') || (p_accu_n>=MAX_VALUE_LEN))
    {
      run_IO_cmd(&p_accu);
      parsed_count++;
      p_accu = "";
      pflag = 0;
    }
    else if(c=='\n')
    {
      p_accu = "";
      pflag = 0;
    }
    else
    {
      p_accu += c;
      p_accu_n++;
    }
  }*/
  else
  {
    if (pflag == 1) //cmd type: set variables
    {
      if (c == '>')
      {
        pflag = 2; //set variable
        cmdtype = c;
      }
      else if (c == '<')
      {
        pflag = 5; //unset variable
        cmdtype = c;
      }
      else if (c == 'T')
      {
        pflag = 6; //set clock time
        cmdtype = c;
      }
      else if (c == 'I')
      {
        pflag = 7; //set inbox message count
        cmdtype = c;
      }
      else
        pflag = 0;
      vars_parse_head = "";
      p_accu = "";
      vars_parse_type = 0;
      p_comma_n = 0;
      p_accu_n = 0;
    }
    else if ((pflag == 6) && (cmdtype == 'T'))
    {
      //Serial.print(c);
      if ((c == ';') || (c == '\n') || (p_accu_n > 50))
      {
        pflag = 0;
        int seg_ind = p_accu.indexOf(',');
        int k = 0;
        unsigned long synced_time_now = 0;
        while (seg_ind > 0)
        {
          if (k == 0)
            synced_time_now = strtoul(p_accu.substring(0, seg_ind).c_str(), NULL, 10);
          else
            time_parts[k - 1] = p_accu.substring(0, seg_ind).toInt();
          
          p_accu = p_accu.substring(seg_ind + 1);
          seg_ind = p_accu.indexOf(',');
          k++;
        }
        if (synced_time_now > 0)
        {
          synced_time = synced_time_now;
          synced_millis = millis();
		  RTC_synced = 1;
#if TTC_INTERNAL_LOGS
          log("[sync] Time: ");
          logln(synced_time_now);
#endif
        }
        p_accu = "";
      }
      else
      {
        p_accu += c;
        p_accu_n++;
      }
    }
    else if ((pflag == 7) && (cmdtype == 'I') )
    {
      if ((c == ';') || (c == '\n') || (p_accu_n > 15))
      {
        if(p_accu_n>0)
        inbox = p_accu.toInt();
        p_accu = "";
        pflag = 0;
      }
      else
      {
        p_accu += c;
        p_accu_n++;
      }
    }
    else if ((pflag == 2) && (cmdtype == '>') && (p_accu_n < MAX_VALUE_LEN))
    {
      if (vars_parse_type == 0)
      {
        if ((c >= 48) && (c <= 122))
        {
          vars_parse_type = c;
        }
        else
          pflag = 0;
      }
      else if ((c == ',') && (p_comma_n < 2))
      {
        if (p_comma_n == 1)
        {
          vars_parse_head = p_accu;
          //Serial.println(vars_parse_head);
          p_accu = "";
          p_accu_n = 0;
          if (vars_parse_head.length() >= MAX_NAME_LEN)
            vars_parse_head = vars_parse_head.substring(0, MAX_NAME_LEN);
        }
        p_comma_n++;
      }
      else if ((c == ';') || (c == '\n')) //cmd end
      {
        //Serial.println(p_accu);
        if (p_comma_n != 2)
          pflag = 0;
        else
        {
          if (set_var(vars_parse_type, &vars_parse_head, &p_accu))
            parsed_count++;

          //Serial.print("[OK]");
          pflag = 0;
          //show_vars();

          vars_parse_head = "";
          p_accu = "";
        }
      }
      else
      {
        p_accu += c;
        p_accu_n++;
      }
    }
    else if ((pflag == 5) && (cmdtype == '<'))
    {
      if ((c == ';') || (c == '\n'))
      {
        if (p_accu.length() >= MAX_NAME_LEN)
          p_accu = p_accu.substring(0, MAX_NAME_LEN);

        uint8_t key_len = p_accu.length() + 1;
        if (key_len > 1)
        {
          char key_name[MAX_NAME_LEN];
          p_accu.toCharArray(key_name, key_len);
          Clear(key_name);
          //Serial.print("[CLR]");
        }

        p_accu_n = 0;
        p_accu = "";
        pflag = 0;
      }
      else
      {
        p_accu += c;
        p_accu_n++;
      }
    }
    else
      pflag = 0;
  }
}

void TTC::json_buffer_start(char cmdtype)
{
  /*
  T for time
  I for inbox message count
  M for messages in inbox
  V for variables
  D for push data
  P for IOs (disabled)
  N for clear NT
  S for sleep
  U for update
  R for restart
  C for clear variables
  */
  if (cmdtype == 'V') //send variables
  {
    reply_type = cmdtype;
    reply_flag = 1;
  }
  else if (cmdtype == 'N') //clear node token
  {
#if TTC_INTERNAL_LOGS
    logln(F("[cmd] ack. NT clear"));
#endif
    node_token_valid = 0;
	first_sync_complete = 0;
    set_NODE_TOKEN("00000000000000000000000");
  }
  else if (cmdtype == 'E') //Clock reset
  {
#if TTC_INTERNAL_LOGS
    logln(F("[cmd] ack. Clock clear"));
#endif
	first_sync_complete = 0;
	synced_time = 0;
	synced_millis = 0;
  }
  else if (cmdtype == 'S') //Sleep
  {
#if TTC_INTERNAL_LOGS
    logln(F("[cmd] ack. 12h deep sleep."));
#endif
    delay(10);
#if BOARD_ESP > 0
    CommitLogs(WiFi.macAddress().c_str());
    ESP.deepSleep(432e8); //12h=12*60*60*1000000
#endif
  }
  else if (cmdtype == 'U') //update. Not done yet.
  {
#if TTC_INTERNAL_LOGS
    logln(F("[cmd] ack. Update. (beta)"));
#endif
  }
  else if ((cmdtype == 'R') || (cmdtype == 'C')) //Restart or clear
  {
    if (cmdtype == 'C')
    {
		first_sync_complete = 0;
      ClearAllVariables();
#if TTC_INTERNAL_LOGS
      logln(F("[cmd] ack. Clear."));
#endif
    }
    else
    {
#if TTC_INTERNAL_LOGS
      logln(F("[cmd] ack. Restart."));
#endif
      delay(10);
#if BOARD_ESP > 0
      CommitLogs(WiFi.macAddress().c_str());
      ESP.restart();
#endif
    }
  }
  else if (cmdtype == 'D') //Push Data
  {
    reply_type = cmdtype;
    reply_flag = 1;
  }
  else if (cmdtype == 'P') //Send IO Pins
  {
    //send "{" + reply_io + "}"
    reply_io = "";
  }
}

bool TTC::json_buffer_available(void)
{
  if (reply_flag > 0)
  {
    if (reply_flag == 10)
    {
      //complete
      reply_flag = 0;
      return 0;
    }
    else
      return 1;
  }
  else
    return 0;
}

char TTC::json_buffer_read_char(void)
{
  if (reply_type == 'V')
  {
    if (reply_flag == 1)
    {
      rep_i = 0;
      reply_accum = "";
      reply_flag = 2;
      var_first = 1;
      return '{';
    }
    else if (reply_flag == 2)
    {
      while (rep_i < MAX_VARS)
      {
        if ((vars[rep_i].type >= 48) && (vars[rep_i].type <= 122))
        {
          reply_accum = jsonify_var(rep_i);
          vars_s_len = reply_accum.length();
          if (vars_s_len > 2)
          {
            reply_flag = 3;
            vars_s = 0;
            rep_i++;
            break;
          }
          else
            rep_i++;
        }
        else
          rep_i++;
      }
      if ((reply_flag != 3) && (reply_flag != 10))
      {
        reply_type = '\0';
        reply_flag = 10;
        return '}';
      }
      else if (reply_flag == 10)
      {
        reply_flag = 0;
        return ';';
      }
      else if (var_first)
      {
        var_first = 0;
        return ' ';
      }
      else
        return ',';
    }
    else if (reply_flag == 3)
    {
      vars_s++;
      if (vars_s <= vars_s_len)
        return reply_accum[vars_s - 1];
      else
      {
        reply_flag = 2;
        return ' ';
      };
    }
  }
  else if (reply_type == 'D')
  {
    if (reply_flag == 1)
    {
      rep_i = 0;
      reply_accum = "";
      reply_flag = 21;
      while (rep_i < MAX_DATA_ENTRIES)
      {
        if (DataIsValid(rep_i))
          break;
        else
          rep_i++;
      }
      batch_s = DataBatchNo(rep_i);
      return '{'; //data open
    }
    else if (reply_flag == 21) //send batch no
    {
      reply_flag++;
      return '"';
    }
    else if (reply_flag == 22) //send batch no
    {
      var_first = 1;
      reply_flag++;
      return (char)(97 + batch_s); //acii 'a'=97
    }
    else if (reply_flag == 23) //send batch no
    {
      reply_flag++;
      return '"';
    }
    else if (reply_flag == 24)
    {
      reply_flag++;
      return ':';
    }
    else if (reply_flag == 25)
    {
      reply_flag = 2;
      return '{'; //batch open bracket
    }

    else if (reply_flag == 30) //batch change
    {
      reply_flag = 21;
      return ','; //between batches
    }
    else if (reply_flag == 9)
    {
      reply_type = '\0';
      reply_flag = 10; //data end bracket
      return '}';
    }
    else if (reply_flag == 2)
    {
      while (rep_i < MAX_DATA_ENTRIES)
      {
        if (DataIsValid(rep_i))
        {
          if (DataBatchNo(rep_i) == batch_s)
          {
            reply_accum = jsonify_data(rep_i);
            vars_s_len = reply_accum.length();
            if (vars_s_len > 2)
            {
              reply_flag = 3;
              vars_s = 0;
              rep_i++;
              break;
            }
            else
              rep_i++;
          }
          else //batch changed
          {
            batch_s = DataBatchNo(rep_i);
            reply_flag = 30;
            return '}';
          }
        }
        else
          rep_i++;
      }
      if ((reply_flag != 3) && (reply_flag != 10))
      {
        reply_flag = 9;
        return '}'; //batch close
      }
      else if (reply_flag == 10)
      {
        reply_flag = 0;
        return ';';
      }
      else if (var_first)
      {
        var_first = 0;
        return ' ';
      }
      else
        return ',';
    }
    else if (reply_flag == 3)
    {
      vars_s++;
      if (vars_s <= vars_s_len)
        return reply_accum[vars_s - 1];
      else
      {
        reply_flag = 2;
        return ' ';
      };
    }
  }
  else
    return ' ';
}

String TTC::jsonify_var(uint8_t i)
{
  String js = "";
  if ((vars[i].type >= 48) && (vars[i].type <= 122))
  {
    js = "\"";
    js += String(vars[i].keyword);
    js += "\":{\"k\":\"";
    js += vars[i].type;
    if (((vars[i].type == 'i') || (vars[i].type == 'l') || (vars[i].type == 'u') || (vars[i].type == 'f') || (vars[i].type == 'b')) && (vars[i].value.length() > 0))
    {
      js += "\",\"v\": ";
      js += vars[i].value;
      js += "}";
    }
    else //string or hex type
    {
      js += "\",\"v\":\"";
      js += vars[i].value;
      js += "\"}";
    }
  }
  return js;
}

/*
  void TTC::run_IO_cmd(String *cmdtxt)
  {
  //_PA2=12;

  if((*cmdtxt)[0]=='P')
  {
    char io_type = (*cmdtxt)[1];
    if((io_type=='A') || (io_type=='D'))
    {
      int eq_ind = cmdtxt->indexOf('=');
      int io_value = 0;
      int pin_no = -1;
      if(eq_ind>2)  //set IO value
      {
        pin_no = cmdtxt->substring(2, eq_ind).toInt();
        io_value = cmdtxt->substring(eq_ind+1).toInt();
        if((pin_no>=0)  && (pin_no<255) )
        {
          if (io_type=='A')
            analogWrite(pin_no, io_value);
          else
          {
            //pinMode(pin_no, OUTPUT);
            digitalWrite(pin_no, io_value);
          }
        }
      }
      else
      {
        pin_no = cmdtxt->substring(2).toInt();
        if((pin_no>=0)  && (pin_no<255))
        {
          if (io_type=='A')
            io_value = analogRead(pin_no);
          else
          {
            //pinMode(pin_no, INPUT);
            io_value = digitalRead(pin_no);
          }
        }
      }
      uint16_t s_io_len = reply_io.length();
      if(s_io_len>20) {s_io_len = 0; reply_io="";}
      if(s_io_len>0) reply_io += ',';

      reply_io += "\"";
      reply_io += String(io_type);
      reply_io += String(pin_no) ;
      reply_io += "\":" ;
      reply_io += String(io_value);
      logln(reply_io);
    }
  }
  }
*/

void TTC::cryptSetKey(const char *secret) //max 15 chars
{
  sec_key = (byte *)malloc((USER_SEC_LENGTH * sizeof(byte)) + 1);

  for (uint8_t i = 0; i < USER_SEC_LENGTH; i++)
  {
    if (secret[i] == '\0')
    {
      sec_key[i] = '\0';
      break;
    }
    else
    {
      sec_key[i] = secret[i] + (((i * cip[5])) % 2 ? ((i * cip[i % 6]) % 255) : ((i * cip[i % 7]) % 255));
      key_len++;
      //Serial.print((uint8_t)sec_key[i], DEC);
      //Serial.print(',');
    }
  }
  sec_key[USER_SEC_LENGTH] = '\0';
  //Serial.println("TT_enc");
}

void TTC::enc_start()
{
  enc_c = sec_key[0] % key_len;
  enc_i = sec_key[1];
  enc_last = 99;
  enc_sum = 0;
}

void TTC::dec_start()
{
  dec_c = sec_key[0] % key_len;
  dec_i = sec_key[1];
  dec_last = 99;
  dec_sum = 0;
}

byte *TTC::enc_ctring(char *sc)
{
  byte *ret_buff = NULL;

  uint8_t buff_len = 0;
  for (uint8_t i = 0; i < 256; i++)
  {
    if (sc[i] == '\0')
      break;
    else
      buff_len++;
  }

  ret_buff = (byte *)malloc((buff_len * sizeof(byte)) + 1);

  for (uint8_t i = 0; i < 256; i++)
  {
    ret_buff[i] = enc_char(sc[i]);
    if (sc[i] == '\0')
      break;
  }
  return ret_buff;
}

byte TTC::enc_char(char c)
{
  enc_c++;
  enc_i++;
  if (sec_key[enc_c] == '\0')
    enc_c = 0;

  byte ret = (byte)(((c + sec_key[enc_c] - enc_last - enc_sum) % 256) + cip[enc_i % 7]) % 256;
  enc_last = (byte)c;
  enc_sum += enc_last;
  enc_last %= 256;
  return ret;
}

char TTC::dec_char(byte c)
{
  dec_c++;
  dec_i++;
  if (sec_key[dec_c] == '\0')
    dec_c = 0;
  int sums = (c - sec_key[dec_c] + dec_last + dec_sum - cip[dec_i % 7]);
  while (sums < 0)
    sums += 256;
  char ret = (char)(sums % 256);

  dec_last = (byte)ret;
  dec_sum += dec_last;
  dec_last %= 256;
  return ret;
}

char *TTC::byte2hex(byte val) //not used
{
  char *hex_buff = NULL;
  hex_buff = (char *)malloc(1 * sizeof(char));
  //input binary(1 byte/byte) to Hex chars (2 chars/byte)

  byte nib1 = (val >> 4) & 0x0F;
  byte nib2 = (val >> 0) & 0x0F;
  hex_buff[0] = nib1 < 0xA ? '0' + nib1 : 'A' + nib1 - 0xA;
  hex_buff[1] = nib2 < 0xA ? '0' + nib2 : 'A' + nib2 - 0xA;
  //hex_buff[2] = '\0';
  return hex_buff;
}

byte *TTC::hex2bytes(char *hex_in, uint16_t hex_len) //not used
{
  //convert String hex=FF to byte hex 0xFF
  byte *ret_arr = NULL;

  uint16_t len = hex_len;
  uint16_t bn = len / 2;

  // try to allocate memory
  ret_arr = (byte *)malloc(bn * sizeof(byte));

  while (len >= 2)
  {
    bn -= 1;
    char c2[2];
    c2[0] = hex_in[len - 2];
    c2[1] = hex_in[len - 1];
    ret_arr[bn] = strtoul(c2, NULL, 16);
    //ret[bn] = strtoul("E8", NULL, 16);
    len -= 2;
  }
  //byte ret[]= {0x28, 0xFF, 0xC7, 0xBA, 0x42, 0x16, 0x04, 0xE8 };

  return ret_arr;
}

int TTC::CommitData(TCPClientObj *TCPclient)
{
  if (!node_token_valid)
    return ERR_NO_NODE_TOKEN;

#if TTC_INTERNAL_LOGS
  log(F("[data] Connecting\t"));
#endif
  //WiFiClient TCPclient;
  if (!TCPclient->connect(_TTC_server, TT_TCP_port))
  {
    TCP_fails++;
#if TTC_INTERNAL_LOGS
    log(F("Failed\t"));
    logln(TCP_fails);
#endif
    if (TCP_fails >= 10)
      return ERR_FAILED_CONN_10x;
    return ERR_FAILED_CONN;
  }
  else
  {
    TCP_fails = 0;
#if TTC_INTERNAL_LOGS
    logln("OK");
#endif
    unsigned long tcp_start_time = millis();

    // first authenticate using this format:
    //   AUTH=_USER_TOK;\b\rNT=NODE_TOKEN;
    //   Everything after \b\r should be sent with encryption by enc_char()
    //   If there is no node token assigned to this node yet, then use Identifier to get the node token by MAC address.
	/*
    TCPclient->printf("AUTH=%s;\b\r", _USER_TOK);
    //encryption starts with \b\r, everything after this should be encrypted by USER_SECRET
    enc_start();
    delay(10); //important pause

    TCP_send_enc(TCPclient, String(tcp_start_time));
    TCPclient->write(enc_char('_'));
    TCP_send_enc(TCPclient, String(RTC_synced > 0 ? realtime() : 0));
    TCPclient->write(enc_char('&'));
    TCPclient->write(enc_char('\b'));
    //first thing to send right after encryption starts is the NT
    //TCP_send_enc(TCPclient, "NT=");
    for (uint8_t i = 0; i < TOKEN_LENGTH; i++)
      TCPclient->write(enc_char(NT[i]));
    TCPclient->write(enc_char(';'));
    TCPclient->write(enc_char('\n')); //new line, necessary
	*/
	
	Start_AUTH(TCPclient);

    //Now send the rest of the request
    // use 'TCP_send_enc' to send Strings. Use TCPclient.write(enc_char(' ')) to send single characters
    TCP_send_enc(TCPclient, "{\"data\":");

    json_buffer_start('D'); //start json of variables to send
    while (json_buffer_available())
    {
      TCPclient->write(enc_char(json_buffer_read_char()));
    }

    TCP_send_enc(TCPclient, ", \"t_last\":");
    TCP_send_enc(TCPclient, String(millis()));

    TCPclient->write(enc_char('}'));
    TCPclient->write(enc_char('\n')); //"write" only sends single chars, cannot use print
    TCPclient->write(enc_char('\n')); //must send double new line as a packet end indicator
                                      //Serial.println(F("Conn req sent"));

    int ack_reply = 0;
    bool dec_started = 0;
    failed_data_ack++;
    char last_in = '\0';
    String resp = "";

    while ((millis() - tcp_start_time) < TTC_connection_timeout) //timeout
    {
      if (TCPclient->available())
      {
        char ch = static_cast<char>(TCPclient->read());
        if (!dec_started)
        {
          if ((last_in == '\b') && (ch == '\r')) //decryption start indication
          {
            dec_start();
            dec_started = 1;
            //Serial.print("\n[D]\n");
          }
          last_in = ch;
        }
        else
        {
          char dec_c = dec_char(ch);

          if ((last_in == 'O') && (dec_c == 'K')) //stream end indication
          {
            ack_reply = 1;
            failed_data_ack = 0;
#if TTC_INTERNAL_LOGS
            logln(F("[data] ack"));
#endif
            DataClear();
          }
          else if ((dec_c == '*') && (last_in == '\n')) //\n*ERROR
          {
#if TTC_INTERNAL_LOGS
            logln(F("[data] syntax error"));
#endif
            ack_reply = ERR_DATA_PARSE;
            if (_print_logs)
            {
              Serial.println("[data] Data:");
              for (uint8_t d = 0; d < data_i; d++)
                Serial.println(jsonify_data(d));
            }
            DataClear();
          }
          else if ((last_in == '\n') && (dec_c == '\b')) //stream end indication
          {
#if TTC_INTERNAL_LOGS
            if (ack_reply != 1)
              logln(resp);
#endif
            resp = "";
            break;
          }
          else
            resp += dec_c;

          last_in = dec_c;
        }
      }
    }
    TCPclient->stop();
    TCPclient->flush();
    //Serial.println(F("Conn stop."));

    if (failed_data_ack > 3)
	{
      DataClear();
	}

    return ack_reply;
  }
  return ERR_FAILED_CONN;
}

int TTC::SendJSON(TCPClientObj *TCPclient, String data_json)
{
  if (!node_token_valid)
    return ERR_NO_NODE_TOKEN;

#if TTC_INTERNAL_LOGS
  log(F("[data] Connecting\t"));
#endif
  //WiFiClient TCPclient;
  if (!TCPclient->connect(_TTC_server, TT_TCP_port))
  {
    TCP_fails++;
#if TTC_INTERNAL_LOGS
    log(F("Failed\t"));
    logln(TCP_fails);
#endif
    if (TCP_fails >= 10)
      return ERR_FAILED_CONN_10x;
    return ERR_FAILED_CONN;
  }
  else
  {
    TCP_fails = 0;
#if TTC_INTERNAL_LOGS
    logln("OK");
#endif
    unsigned long tcp_start_time = millis();

    // first authenticate using this format:
    //   AUTH=_USER_TOK;\b\rNT=NODE_TOKEN;
    //   Everything after \b\r should be sent with encryption by enc_char()
    //   If there is no node token assigned to this node yet, then use Identifier to get the node token by MAC address.
	/*
    TCPclient->printf("AUTH=%s;\b\r", _USER_TOK);
    //encryption starts with \b\r, everything after this should be encrypted by USER_SECRET
    enc_start();
    delay(10); //important pause

    TCP_send_enc(TCPclient, String(tcp_start_time));
    TCPclient->write(enc_char('_'));
    TCP_send_enc(TCPclient, String(RTC_synced > 0 ? realtime() : 0));
    TCPclient->write(enc_char('&'));
    TCPclient->write(enc_char('\b'));

    //first thing to send right after encryption starts is the NT
    //TCP_send_enc(TCPclient, "NT=");
    for (uint8_t i = 0; i < TOKEN_LENGTH; i++)
      TCPclient->write(enc_char(NT[i]));
    TCPclient->write(enc_char(';'));
    TCPclient->write(enc_char('\n')); //new line, necessary
	*/
	Start_AUTH(TCPclient);

    //Now send the rest of the request
    // use 'TCP_send_enc' to send Strings. Use TCPclient.write(enc_char(' ')) to send single characters
    TCP_send_enc(TCPclient, "{\"data\":");

    TCP_send_enc(TCPclient, data_json);

    TCPclient->write(enc_char('}'));
    TCPclient->write(enc_char('\n')); //"write" only sends single chars, cannot use print
    TCPclient->write(enc_char('\n')); //must send double new line as a packet end indicator
                                      //Serial.println(F("Conn req sent"));

    int ack_reply = 0;
    bool dec_started = 0;
    char last_in = '\0';
    String resp = "";

    while ((millis() - tcp_start_time) < TTC_connection_timeout) //timeout
    {
      if (TCPclient->available())
      {
        char ch = static_cast<char>(TCPclient->read());
        if (!dec_started)
        {
          if ((last_in == '\b') && (ch == '\r')) //decryption start indication
          {
            dec_start();
            dec_started = 1;
            //Serial.print("\n[D]\n");
          }
          last_in = ch;
        }
        else
        {
          char dec_c = dec_char(ch);

          if ((last_in == 'O') && (dec_c == 'K')) //stream end indication
          {
            ack_reply = 1;
#if TTC_INTERNAL_LOGS
            logln(F("[data] ack"));
#endif
          }
          else if ((dec_c == '*') && (last_in == '\n')) // \n*ERROR
          {
#if TTC_INTERNAL_LOGS
            logln(F("[data] syntax error"));
#endif
            ack_reply = ERR_DATA_PARSE;
            if (_print_logs)
            {
              Serial.println("[data] Data:");
              Serial.println(data_json);
            }
          }
          else if ((last_in == '\n') && (dec_c == '\b')) //stream end indication
          {
#if TTC_INTERNAL_LOGS
            if (ack_reply != 1)
              logln(resp);
#endif
            resp = "";
            break;
          }
          else
            resp += dec_c;

          last_in = dec_c;
        }
      }
    }
    TCPclient->stop();
    TCPclient->flush();
    //Serial.println(F("Conn stop."));

    return ack_reply;
  }
  return ERR_FAILED_CONN;
}



String TTC::readMessage(TCPClientObj *TCPclient, char read_order)
{
  String resp = "";

  if (!node_token_valid)
    return resp;//ERR_NO_NODE_TOKEN;

#if TTC_INTERNAL_LOGS
  log(F("[INX] Connecting\t"));
#endif
  //WiFiClient TCPclient;
  if (!TCPclient->connect(_TTC_server, TT_TCP_port))
  {
    TCP_fails++;
#if TTC_INTERNAL_LOGS
    log(F("Failed\t"));
    logln(TCP_fails);
#endif
    if (TCP_fails >= 10)
      return resp;//ERR_FAILED_CONN_10x;
    return resp;//ERR_FAILED_CONN;
  }
  else
  {
    TCP_fails = 0;
#if TTC_INTERNAL_LOGS
    logln("OK");
#endif
    unsigned long tcp_start_time = millis();

	Start_AUTH(TCPclient);

    //Now send the rest of the request
    // use 'TCP_send_enc' to send Strings. Use TCPclient.write(enc_char(' ')) to send single characters
    TCP_send_enc(TCPclient, "{\"inbox\":\"");
	TCPclient->write(enc_char(read_order));
    TCPclient->write(enc_char('\"'));
    TCPclient->write(enc_char('}'));
	
    TCPclient->write(enc_char('\n')); //"write" only sends single chars, cannot use print
    TCPclient->write(enc_char('\n')); //must send double new line as a packet end indicator
                                      //Serial.println(F("Conn req sent"));

    int ack_reply = 0;
    bool dec_started = 0;
    char last_in = '\0';
    while ((millis() - tcp_start_time) < TTC_connection_timeout) //timeout
    {
      if (TCPclient->available())
      {
        char ch = static_cast<char>(TCPclient->read());
        if (!dec_started)
        {
          if ((last_in == '\b') && (ch == '\r')) //decryption start indication
          {
            dec_start();
            dec_started = 1;
            //Serial.print("\n[D]\n");
          }
          last_in = ch;
        }
        else
        {
          char dec_c = dec_char(ch);

          if ((last_in == '\b') && (dec_c == 'K')) //different ACK instead of OK
          {
            ack_reply = 1;
			resp = "";
#if TTC_INTERNAL_LOGS
            logln(F("[INX] ack"));
#endif
          }
          else if ((dec_c == '*') && (last_in == '\n')) // \n*ERROR
          {
#if TTC_INTERNAL_LOGS
            logln(F("[INX] Error"));
#endif
            ack_reply = ERR_DATA_PARSE;
          }
          else if ((last_in == '\n') && (dec_c == '\b')) //stream end indication
          {
#if TTC_INTERNAL_LOGS
            if (ack_reply != 1)
              logln(resp);
#endif
            //resp = "";
            break;
          }
          else
            resp += dec_c;

          last_in = dec_c;
        }
      }
    }
    TCPclient->stop();
    TCPclient->flush();
    //Serial.println(F("Conn stop."));

    return resp;//ack_reply;
  }
  return resp;//ERR_FAILED_CONN;
}


int TTC::sendMessage(TCPClientObj *TCPclient, String to, String msg_text)
{
  if (!node_token_valid)
    return ERR_NO_NODE_TOKEN;

#if TTC_INTERNAL_LOGS
  log(F("[COM] Connecting\t"));
#endif
  //WiFiClient TCPclient;
  if (!TCPclient->connect(_TTC_server, TT_TCP_port))
  {
    TCP_fails++;
#if TTC_INTERNAL_LOGS
    log(F("Failed\t"));
    logln(TCP_fails);
#endif
    if (TCP_fails >= 10)
      return ERR_FAILED_CONN_10x;
    return ERR_FAILED_CONN;
  }
  else
  {
    TCP_fails = 0;
#if TTC_INTERNAL_LOGS
    logln("OK");
#endif
    unsigned long tcp_start_time = millis();

	Start_AUTH(TCPclient);

    //Now send the rest of the request
    // use 'TCP_send_enc' to send Strings. Use TCPclient.write(enc_char(' ')) to send single characters
    TCP_send_enc(TCPclient, "{\"to\":\"");
    TCP_send_enc(TCPclient, to);
	TCP_send_enc(TCPclient, "\", \"com\":\"");
	TCP_send_enc(TCPclient, msg_text);
    TCPclient->write(enc_char('\"'));
    TCPclient->write(enc_char('}'));
	
    TCPclient->write(enc_char('\n')); //"write" only sends single chars, cannot use print
    TCPclient->write(enc_char('\n')); //must send double new line as a packet end indicator
                                      //Serial.println(F("Conn req sent"));

    int ack_reply = 0;
    bool dec_started = 0;
    char last_in = '\0';
    String resp = "";

    while ((millis() - tcp_start_time) < TTC_connection_timeout) //timeout
    {
      if (TCPclient->available())
      {
        char ch = static_cast<char>(TCPclient->read());
        if (!dec_started)
        {
          if ((last_in == '\b') && (ch == '\r')) //decryption start indication
          {
            dec_start();
            dec_started = 1;
            //Serial.print("\n[D]\n");
          }
          last_in = ch;
        }
        else
        {
          char dec_c = dec_char(ch);

          if ((last_in == 'O') && (dec_c == 'K')) //stream end indication
          {
            ack_reply = 1;
#if TTC_INTERNAL_LOGS
            logln(F("[COM] ack"));
#endif
          }
          else if ((dec_c == '*') && (last_in == '\n')) // \n*ERROR
          {
#if TTC_INTERNAL_LOGS
            logln(F("[COM] Error"));
#endif
            ack_reply = ERR_DATA_PARSE;
            if (_print_logs)
            {
              Serial.println("[COM] Msg:");
              Serial.println(msg_text);
            }
          }
          else if ((last_in == '\n') && (dec_c == '\b')) //stream end indication
          {
#if TTC_INTERNAL_LOGS
            if (ack_reply != 1)
              logln(resp);
#endif
            resp = "";
            break;
          }
          else
            resp += dec_c;

          last_in = dec_c;
        }
      }
    }
    TCPclient->stop();
    TCPclient->flush();
    //Serial.println(F("Conn stop."));

    return ack_reply;
  }
  return ERR_FAILED_CONN;
}

int TTC::runSync(TCPClientObj *TCPclient)
{
  return runSync(TCPclient, "", "");
}
/*
int TTC::runSync(TCPClientObj *TCPclient, bool skip_send)
{
	first_sync_complete = !skip_send;
    return runSync(TCPclient, "", "");
}*/

int TTC::runSync(TCPClientObj *TCPclient, String script_token, String args_text)
{
  if (!node_token_valid)
    return ERR_NO_NODE_TOKEN;

#if TTC_INTERNAL_LOGS
  log(F("[sync] Connecting\t"));
#endif

  if (!TCPclient->connect(_TTC_server, TT_TCP_port))
  {
    TCP_fails++;
#if TTC_INTERNAL_LOGS
    log(F("Failed\t"));
    logln(TCP_fails);
#endif
    if (TCP_fails >= 10)
      return ERR_FAILED_CONN_10x;
    return ERR_FAILED_CONN;
  }
  else
  {
    set_ulong(RUN_MINUTES, (unsigned long)(millis() / 60000));
    TCP_fails = 0;
#if TTC_INTERNAL_LOGS
    logln("OK");
#endif
    unsigned long tcp_start_time = millis();
	
    /* first authenticate using this format:
       AUTH=_USER_TOK;\b\rNT=NODE_TOKEN;
       Everything after \b\r should be sent with encryption by enc_char()
    */
	/*
    TCPclient->printf("AUTH=%s;\b\r", _USER_TOK);
    //encryption starts with \b\r, everything after this should be encrypted by USER_SECRET
    enc_start();
    delay(10); //important pause
    
    TCP_send_enc(TCPclient, String(tcp_start_time));
    TCPclient->write(enc_char('_'));
    TCP_send_enc(TCPclient, String(RTC_synced > 0 ? realtime() : 0));
    TCPclient->write(enc_char('&'));
    TCPclient->write(enc_char('\b'));

    //first thing to send right after encryption starts is the NT
    //TCP_send_enc(TCPclient, "NT=");
    for (uint8_t i = 0; i < TOKEN_LENGTH; i++)
      TCPclient->write(enc_char(NT[i]));
    TCPclient->write(enc_char(';'));
    TCPclient->write(enc_char('\n')); //new line, necessary
	*/
	Start_AUTH(TCPclient);

    //now send rest of the request (encrypted)
    TCP_send_enc(TCPclient, "{\"variables\":");
    if (first_sync_complete)
    {
      json_buffer_start('V'); //start json of variables to send
      while (json_buffer_available())
      {
        TCPclient->write(enc_char(json_buffer_read_char()));
      }
    }
    else
    {
      TCP_send_enc(TCPclient, "{}");
    }

    //String script_id = get_String(SCRIPT);
    if (script_token.length() == TOKEN_LENGTH)
    {
#if TTC_INTERNAL_LOGS
      log("[sync] script: ");
      logln(script_token);
#endif
      TCP_send_enc(TCPclient, ",\"script\":\"");
      TCP_send_enc(TCPclient, script_token);
      TCPclient->write(enc_char('"'));
	  
	  if (args_text.length() >= 1)
	  {
		TCP_send_enc(TCPclient, ",\"args\":\"");
		TCP_send_enc(TCPclient, args_text);	//format: String args = "example='ABC0000000', a=123";
		TCPclient->write(enc_char('"'));
	  }
	
    }

    TCPclient->write(enc_char('}'));
    TCPclient->write(enc_char('\n')); //"write" only sends single chars, cannot use print
    TCPclient->write(enc_char('\n')); //must send double new line as a packet end indicator
    //Serial.println(F("Conn req sent"));

    bool dec_started = 0;
    vars_parse_start();
    char last_in = '\0';
    bool ack_ok = 0;
    bool error = 0;

    while ((millis() - tcp_start_time) < TTC_connection_timeout) //timeout
    {
      if (TCPclient->available())
      {
        char ch = static_cast<char>(TCPclient->read());
        if (!dec_started)
        {
          if ((last_in == '\b') && (ch == '\r')) //decryption start indication
          {
            dec_start();
            dec_started = 1;

            //Serial.print("\n[D]\n");
          }
          last_in = ch;
        }
        else
        {
          char dec_c = dec_char(ch);

          if ((last_in == '\n') && (dec_c == '\b')) //stream end indication
          {
            ack_ok = 1;
            //Serial.println(F("Conn end ack"));
#if TTC_INTERNAL_LOGS
            logln(F("[sync] ack"));
#endif
            break;
          }
          else if ((dec_c == '*') && (last_in == '\n'))
            error = 1;
          else
          {
            //Serial.print(dec_c);
            vars_char_parse(dec_c); //this function interprets the variables info recieved. Doesn't work with custom requests
            if(error) log(dec_c);
          }
          last_in = dec_c;
        }
      }
    }
    TCPclient->stop();
    TCPclient->flush();

    if (ack_ok)
	{
		if(!error)
		{
			first_sync_complete = 1;
			return vars_parse_count();
		}
		else  
		{
			first_sync_complete = 0;
			return ERR_DATA_PARSE;
		}
	}
    else
	{
		RTC_synced = 0;
        return ERR_ACK_FAILED;
	}
  }
  return ERR_FAILED_CONN;
}


void TTC::Start_AUTH(TCPClientObj *TCPclient_x)
{
	/* first authenticate using this format:
       AUTH=_USER_TOK;\b\rNT=NODE_TOKEN;
       Everything after \b\r should be sent with encryption by enc_char()
    */
    TCPclient_x->printf("AUTH=%s;\b\r", _USER_TOK);
    //encryption starts with \b\r, everything after this should be encrypted by USER_SECRET
    enc_start();
    delay(10); //important pause
    
    TCP_send_enc(TCPclient_x, String(millis()));
    TCPclient_x->write(enc_char('_'));
    TCP_send_enc(TCPclient_x, String(RTC_synced > 0 ? realtime() : 0));
    TCPclient_x->write(enc_char('&'));
    TCPclient_x->write(enc_char('\b'));

    //first thing to send right after encryption starts is the NT
    //TCP_send_enc(TCPclient_x, "NT=");
    for (uint8_t i = 0; i < TOKEN_LENGTH; i++)
      TCPclient_x->write(enc_char(NT[i]));
    TCPclient_x->write(enc_char(';'));
    TCPclient_x->write(enc_char('\n')); //new line, necessary
}


int TTC::Identify(TCPClientObj *TCPclient, String mac_id_str)
{
#if TTC_INTERNAL_LOGS
  log(F("[idnt] Connecting\t"));
#endif
  //WiFiClient TCPclient;
  if (!TCPclient->connect(_TTC_server, TT_TCP_port))
  {
    TCP_fails++;
#if TTC_INTERNAL_LOGS
    log(F("Failed\t"));
    logln(TCP_fails);
#endif
    if (TCP_fails >= 10)
      return ERR_FAILED_CONN_10x;
    return ERR_FAILED_CONN;
  }
  else
  {
    TCP_fails = 0;
#if TTC_INTERNAL_LOGS
    logln(F("OK"));
#endif
    unsigned long tcp_start_time = millis();

    /* first authenticate using this format:
       AUTH=_USER_TOK;\b\rSC=USER_SECRET;
       Everything after \b\r should be sent with encryption by enc_char()
    */
    TCPclient->printf("AUTH=%s;\b\r", _USER_TOK);
	//Serial.printf("AUTH=%s;\b\r", _USER_TOK);
    //encryption starts with \b\r, everything after this should be encrypted by USER_SECRET
    enc_start();
    delay(10);

    TCP_send_enc(TCPclient, String(tcp_start_time));
    TCPclient->write(enc_char('_'));
    TCP_send_enc(TCPclient, String(RTC_synced > 0 ? realtime() : 0));
    TCPclient->write(enc_char('&'));
    TCPclient->write(enc_char('\b'));
    //TCP_send_enc(TCPclient, "SC=");
    TCP_send_enc(TCPclient, _USER_SEC); //first thing to send right after encryption start should be the USER_SECRET
    TCPclient->write(enc_char(';'));
    TCPclient->write(enc_char('\n')); //new line, necessary

    //now send the IDN (Identifier) to get the node token      
    TCP_send_enc(TCPclient, F("{\"IDN\":\"")); //send mac address to get this node's unique token from server
    TCP_send_enc(TCPclient, mac_id_str);
    TCP_send_enc(TCPclient, "\"}");

    TCPclient->write(enc_char('\n')); //"write" only sends single chars, cannot use print
    TCPclient->write(enc_char('\n')); //must send double new line as a packet end indicator
    //Serial.println(F("Conn req sent"));

    bool dec_started = 0;
    bool ack_ok = 0;
    bool error = 0;
    uint8_t NT_cmd_flag = 0;
    //vars_parse_start();
    char last_in = '\0';
    char collect_NT[TOKEN_LENGTH];
    uint8_t NT_i = 0;

    while ((millis() - tcp_start_time) < TTC_connection_timeout) //timeout
    {
      if (TCPclient->available())
      {
        char ch = static_cast<char>(TCPclient->read());
        if (!dec_started)
        {
          if ((last_in == '\b') && (ch == '\r')) //decryption start indication
          {
            dec_start();
            dec_started = 1;
            //Serial.print("\n[D]\n");

#if TTC_INTERNAL_LOGS
            log("[idnt] *D*");
#endif
          }
#if TTC_INTERNAL_LOGS
			else
            log(ch);
#endif
          last_in = ch;
        }
        else
        {
          char dec_c = dec_char(ch);

          if ((last_in == '\n') && (dec_c == '\b')) //stream end indication
          {
            ack_ok = 1;
#if TTC_INTERNAL_LOGS
            logln("[idnt] ack");
#endif
            break;
          }
          else if ((NT_cmd_flag != 4) && (last_in == '$') && (dec_c == '>'))
            NT_cmd_flag = 1;
          else if ((last_in == 'N') && (dec_c == 'T'))
            NT_cmd_flag = 2;
          else if ((NT_cmd_flag == 2) && (dec_c == ','))
          {
            NT_cmd_flag = 3;
            NT_i = 0;
          }
          else if ((NT_cmd_flag == 3) && ((dec_c == ';') || (dec_c == '\n') || (dec_c == ',')))
          {
            if (NT_i == TOKEN_LENGTH)
            {
              NT_cmd_flag = 4;
            }
            else
              NT_cmd_flag = 0;
          }
          else if ((NT_cmd_flag == 3) && (NT_i < TOKEN_LENGTH) && (confirm_ascii(dec_c)))
          {
            collect_NT[NT_i] = dec_c;
            NT_i++;
          }
          else if ((dec_c == '*') && (last_in == '\n')) // \n*ERROR
            error = 1;
          
          if(error)
            log(dec_c);
			
          
          last_in = dec_c;
        }
      }
    }
    TCPclient->stop();
    TCPclient->flush();
    //Serial.println(F("Conn stop."));

    if (NT_cmd_flag == 4)
    {
      if (set_NODE_TOKEN(collect_NT))
      {
#if TTC_INTERNAL_LOGS
        logln("[idnt] Got NT");
#endif
        return 1;
      }
    }
#if TTC_INTERNAL_LOGS
    logln("[idnt] NT Failed");
#endif

    if (ack_ok)
      return 0;
    else
	{
	  first_sync_complete = 0;
	  RTC_synced = 0;
      return ERR_ACK_FAILED;
	}
  }
  return ERR_FAILED_CONN;
}

bool TTC::set_NODE_TOKEN(char *new_NT)
{
  for (uint8_t i = 0; i < TOKEN_LENGTH; i++)
  {
    if (confirm_ascii(new_NT[i]))
      NT[i] = new_NT[i];
    else
    {
      node_token_valid = 0;
      return node_token_valid;
    }
  }
  node_token_valid = 1;
  return node_token_valid;
}

bool TTC::confirm_ascii(char this_char)
{
  if ((this_char >= 48) && (this_char <= 122))
    return 1;
  else
    return 0;
}

void TTC::TCP_send_enc(TCPClientObj *TCPclient_x, String to_send) //use this to print long sentences instead of single characters
{
  for (uint8_t i = 0; i < to_send.length(); i++) //cannot use print because enc_char returns binary bytes
  {
    //if(to_send[i]=='\0') break;
    //Serial.print(to_send[i]);
    TCPclient_x->write(enc_char(to_send[i]));
  }
}

void TTC::log(const char *str)
{
  if (_print_logs)
    Serial.print(str);
  logs_str += str;
  if (logs_str.length() > 200)
    logs_str.substring(10);
}
void TTC::logln(const char *str)
{
  if (_print_logs)
    Serial.println(str);
  logs_str += str;
  logs_str += '\n';
  if (logs_str.length() > 200)
    logs_str = logs_str.substring(10);
}

void TTC::log(String str)
{
  if (_print_logs)
    Serial.print(str);
  logs_str += str;
  if (logs_str.length() > 200)
    logs_str = logs_str.substring(20);
}
void TTC::logln(String str)
{
  if (_print_logs)
    Serial.println(str);
  logs_str += str;
  if (logs_str.length() > 200)
    logs_str = logs_str.substring(20);
  logs_str += '\n';
}

void TTC::log(char c)
{
  if (_print_logs)
    Serial.print(c);
  logs_str += c;
}

void TTC::logln(char c)
{
  if (_print_logs)
    Serial.println(c);
  logs_str += c;
  logs_str += '\n';
}

void TTC::log(int num)
{
  if (_print_logs)
    Serial.print(num);
  logs_str += String(num);
}
void TTC::logln(int num)
{
  if (_print_logs)
    Serial.println(num);
  logs_str += String(num);
  logs_str += '\n';
}

void TTC::log(long num)
{
  if (_print_logs)
    Serial.print(num);
  logs_str += String(num);
}

void TTC::logln(long num)
{
  if (_print_logs)
    Serial.println(num);
  logs_str += String(num);
  logs_str += '\n';
}

void TTC::log(unsigned long num)
{
  if (_print_logs)
    Serial.print(num);
  logs_str += String(num);
}

void TTC::logln(unsigned long num)
{
  if (_print_logs)
    Serial.println(num);
  logs_str += String(num);
  logs_str += '\n';
}

void TTC::log(double num)
{
  if (_print_logs)
    Serial.print(num);
  logs_str += String(num);
}

void TTC::logln(double num)
{
  if (_print_logs)
    Serial.println(num);
  logs_str += String(num);
  logs_str += '\n';
}

void TTC::CommitLogs(const char *identity)
{

  if (logs_str.length() > 2)
  {
    Udp.beginPacket(_TTC_server, TT_UDP_port);
    //Udp.beginPacket("192.168.137.1", 44561);

#if BOARD_ESP == 8266
    Udp.write(_USER_TOK);
    Udp.write('\b');
    Udp.write(identity);
    Udp.write('\n');
    //Udp.write(logs_str.c_str());
    for (uint16_t i = 0; i < logs_str.length(); i++)
      Udp.write(logs_str[i]);
#elif BOARD_ESP == 32
    Udp.print(_USER_TOK);
    Udp.write('\b');
    Udp.print(identity);
    Udp.write('\n');
    for (uint16_t i = 0; i < logs_str.length(); i++)
      Udp.write(logs_str[i]);
      //Udp.print(logs_str.c_str());
#else
#error Architecture is NOT ESP8266 OR ESP32
#endif

    Udp.endPacket();
    logs_str = "";
  }
}

//Update functions

String TTC::fetchUpdateURL(TCPClientObj *TCPclient, const char *TTC_update_server, String use_idn)
{

  delay(1);
  yield();
  //WiFiClient client;

  String payload = "";

#if TTC_INTERNAL_LOGS
  log(F("[upd_url] fetchUpdateURL\t"));
  logln(TTC_update_server);
  log(F("[upd_url] Connecting\t"));
#endif

  if (!TCPclient->connect(TTC_update_server, 80))
  {
#if TTC_INTERNAL_LOGS
    logln(F("Failed"));
#endif
    return payload;
  }
  else
  {
    logln(F("OK"));
    unsigned long up_v_start = millis();

    delay(0);
    TCPclient->print("GET /u/");
    TCPclient->print(_USER_TOK);
    TCPclient->print('/');

    if (use_idn.length() > 1)
    {
#if TTC_INTERNAL_LOGS
      logln(F("[upd_url] Using IDN"));
#endif
      TCPclient->print(use_idn);
    }
    else if (node_token_valid)
    {
      for (uint8_t i = 0; i < TOKEN_LENGTH; i++)
        TCPclient->write(NT[i]);
    }
    else
    {
#if TTC_INTERNAL_LOGS
      logln(F("[upd_url] No NT or IDN for update"));
#endif
      TCPclient->write("0");
    }

    TCPclient->print("/version");
    TCPclient->println(" HTTP/1.1");

    TCPclient->print("Host: ");
    TCPclient->println(TTC_update_server);
    TCPclient->println("Cache-Control: no-cache");
    TCPclient->println("Connection: close");
    TCPclient->println();

    uint8_t js_flag95 = 0;
    while ((millis() - up_v_start) < TTC_connection_timeout)
    {
      while (TCPclient->available())
      {
        char c = TCPclient->read();
        if (c == '{')
          js_flag95 = 1;
        else if (c == '}')
        {
          js_flag95 = 0;
          break;
        }
        else if (js_flag95 == 1 && c == 'b')
          js_flag95 = 2;
        else if (js_flag95 == 2 && c == ':')
          js_flag95 = 3;
        else if (js_flag95 == 3 && c == '"')
        {
          js_flag95 = 4;
          payload = "";
        }
        else if (js_flag95 == 4)
        {
          if (c == '"')
          {
            js_flag95 = 5;
            break;
          }
          else
            payload += c;
        }
        else
          payload += c;
      }
      if ((js_flag95 == 5) && (payload.indexOf("http://") >= 0) && (payload.indexOf(".bin") > 0))
      {
        //logln("Found .bin");
        break;
      }
      else if (payload.indexOf("NOT FOUND") >= 0)
      {
        //logln(".bin not available");
        payload = "0";
        break;
      }
      else if (payload.indexOf("ERROR") >= 0)
      {
        payload = payload.substring(payload.indexOf("ERR"));
        //logln(payload);
        payload = "ERROR";
        break;
      }
      else if (payload.length() > 110)
        payload = payload.substring(5);
    }
    TCPclient->stop();
  }
  if (payload.indexOf("http://") > 0)
    payload = payload.substring(payload.indexOf("http://"));

  return payload;
}

#if defined(ENABLE_OTA_UPDATE) && (ENABLE_OTA_UPDATE == 1)

String TTC::getHeaderValue(String header, String headerName)
{
  return header.substring(strlen(headerName.c_str()));
}

#if BOARD_ESP == 32

#include <Update.h>

bool TTC::executeOTAupdate(String bin_link)
{

  int slash_ind = bin_link.indexOf("//");
  if (slash_ind < 3)
  {
    Serial.println(F("[UPD] Invalid link"));
    return false;
  }

  String bin_path = bin_link.substring(slash_ind + 2); // Host => bucket-name.s3.region.amazonaws.com

  slash_ind = bin_path.indexOf('/');
  if (slash_ind < 3)
  {
    Serial.println(F("[UPD] Invalid link"));
    return false;
  }

  String bin_host = bin_path.substring(0, slash_ind);
  bin_path = bin_path.substring(slash_ind);

  TCPClientObj TCPclient;
  Serial.println("[UPD] Connecting to: " + String(bin_host));

  long bin_contentLength = 0;
  bool bin_isValidContentType = false;

  if (TCPclient.connect(bin_host.c_str(), 80))
  {

    Serial.println("[UPD] Fetching Bin: " + bin_path);

    // Get the contents of the bin file
    TCPclient.print(String("GET ") + bin_path + " HTTP/1.1\r\n" +
                    "Host: " + bin_host + "\r\n" +
                    "Cache-Control: no-cache\r\n" +
                    "Connection: close\r\n\r\n");

    unsigned long timeout = millis();
    while (TCPclient.available() == 0)
    {
      if (millis() - timeout > 9000)
      {
        Serial.println("[UPD] Client Timeout !");
        TCPclient.stop();
        return false;
      }
    }
    // Once the response is available,
    // check stuff

    /*
       Response Structure
        HTTP/1.1 200 OK
        x-amz-id-2: NVKxnU1aIQMmpGKhSwpCBh8y2JPbak18QLIfE+OiUDOos+7UftZKjtCFqrwsGOZRN5Zee0jpTd0=
        x-amz-request-id: 2D56B47560B764EC
        Date: Wed, 14 Jun 2017 03:33:59 GMT
        Last-Modified: Fri, 02 Jun 2017 14:50:11 GMT
        ETag: "d2afebbaaebc38cd669ce36727152af9"
        Accept-Ranges: bytes
        Content-Type: application/octet-stream
        Content-Length: 357280
        Server: AmazonS3

        {{BIN FILE CONTENTS}}

    */
    while (TCPclient.available())
    {
      // read line till /n
      String line = TCPclient.readStringUntil('\n');
      // remove space, to check if the line is end of headers
      line.trim();

      // if the the line is empty,
      // this is end of headers
      // break the while and feed the
      // remaining `TCPclient` to the
      // Update.writeStream();
      if (!line.length())
      {
        //headers ended
        break; // and get the OTA started
      }

      // Check if the HTTP Response is 200
      // else break and Exit Update
      if (line.startsWith("HTTP/1.1"))
      {
        if (line.indexOf("200") < 0)
        {
          Serial.println(F("[UPD] Non-200 status code from server. Exiting OTA Update."));
          break;
        }
      }
      if (line.startsWith("Content-Length: "))
      {
        bin_contentLength = atol((getHeaderValue(line, "Content-Length: ")).c_str());
        Serial.println("[UPD] Got " + String(bin_contentLength) + " bytes from server");
      }

      if (line.startsWith("Content-Type: "))
      {
        String contentType = getHeaderValue(line, "Content-Type: ");
        Serial.println("[UPD] Got " + contentType + " payload.");
        if (contentType == "application/octet-stream")
        {
          bin_isValidContentType = true;
        }
      }
    }
  }
  else
  {
    Serial.println("[UPD] Connection to " + String(bin_host) + " failed.");
  }

  Serial.println("[UPD] len : " + String(bin_contentLength) + ", valid : " + String(bin_isValidContentType));

  // check bin_contentLength and content type
  if (bin_contentLength && bin_isValidContentType)
  {
    // Check if there is enough to OTA Update
    bool canBegin = Update.begin(bin_contentLength);

    // If yes, begin
    if (canBegin)
    {
      Serial.println(F("[UPD] Begin OTA. This may take 2 - 5 mins to complete."));
      // No activity would appear on the Serial monitor
      // So be patient. This may take 2 - 5mins to complete
      size_t written = Update.writeStream(TCPclient);

      if (written == bin_contentLength)
      {
        Serial.println("[UPD] Written : " + String(written) + " successfully");
      }
      else
      {
        Serial.println("[UPD] Written only : " + String(written) + "/" + String(bin_contentLength) + ". Retry?");
        // retry??
        // execOTA();
      }

      if (Update.end())
      {
        Serial.println("[UPD] OTA done!");
        if (Update.isFinished())
        {
          Serial.println("[UPD] Update success. Rebooting.");
          ESP.restart();
        }
        else
        {
          Serial.println(F("[UPD] Something went wrong!"));
        }
      }
      else
      {
        Serial.println("[UPD] Error #: " + String(Update.getError()));
      }
    }
    else
    {
      Serial.println(F("[UPD] Not enough space"));
      TCPclient.stop();
      //TCPclient.flush();
    }
  }
  else
  {
    Serial.println(F("[UPD] empty response"));
    TCPclient.stop();
    //TCPclient.flush();
  }
  return false;
}

#elif BOARD_ESP == 8266

#include <ESP8266httpUpdate.h>
bool TTC::executeOTAupdate(String bin_link)
{
  yield();
  Serial.println("[UPD] Updating");
  delay(1);
  TCPClientObj TCPClient;
  t_httpUpdate_return ret = ESPhttpUpdate.update(TCPClient, bin_link);

  switch (ret)
  {
  case HTTP_UPDATE_FAILED:
    Serial.print(F("[UPD] HTTP_UPDATE_FAILED Error"));
    Serial.print(ESPhttpUpdate.getLastError());
    Serial.println(ESPhttpUpdate.getLastErrorString());
    //Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
    break;

  case HTTP_UPDATE_NO_UPDATES:
    Serial.println(F("[UPD] HTTP_UPDATE_NO_UPDATES"));
    break;

  case HTTP_UPDATE_OK:
    Serial.println(F("[UPD] HTTP_UPDATE_OK")); //it will restart after this
    return true;
    break;
  }
  delay(1);
  return false;
}
#endif

#endif
//weather forecast functions:


int TTC::fetchSunrise(TCPClientObj *TCPclient, String geo_lat, String geo_lon)
{
  if (!node_token_valid)
    return ERR_NO_NODE_TOKEN;

#if TTC_INTERNAL_LOGS
  log(F("[sunr] Connecting\t"));
#endif
  //WiFiClient TCPclient;
  if (!TCPclient->connect(_TTC_server, TT_TCP_port))
  {
    TCP_fails++;
#if TTC_INTERNAL_LOGS
    log(F("Failed\t"));
    logln(TCP_fails);
#endif
    if (TCP_fails >= 10)
      return ERR_FAILED_CONN_10x;
    return ERR_FAILED_CONN;
  }
  else
  {
    TCP_fails = 0;
    logln(F("OK"));

    unsigned long tcp_start_time = millis();

    String custom_request = "{\"sunrise\":{\"lat\":";
    custom_request += geo_lat;
    custom_request += ",\"lon\":";
    custom_request += geo_lon;
    custom_request += ",\"days\":1}}";
    //custom_request += String(forecast_days);
    //custom_request += "}}";

    //Serial.println(custom_request);

    Start_AUTH(TCPclient);

    //Now send the rest of the request
    // use 'TCP_send_enc' to send Strings. Use TCPclient.write(enc_char(' ')) to send single characters
    TCP_send_enc(TCPclient, custom_request);

    TCPclient->write(enc_char('\n')); //"write" only sends single chars, cannot use print
    TCPclient->write(enc_char('\n')); //must send double new line as a packet end indicator
    //Serial.println(F("Conn req sent"));

    weather_parsing_reset();
    bool dec_started = 0;
    bool ack_ok = 0;
    bool error = 0;
    char last_in = '\0';

    while ((millis() - tcp_start_time) < TTC_connection_timeout) //timeout
    {
      if (TCPclient->available())
      {
        char ch = static_cast<char>(TCPclient->read());
        if (!dec_started)
        {
          if ((last_in == '\b') && (ch == '\r')) //decryption start indication
          {
            dec_start();
            dec_started = 1;
            //Serial.print("\n[D]\n");
          }
          last_in = ch;
        }
        else
        {
          char dec_c = dec_char(ch);

          if ((last_in == '\n') && (dec_c == '\b')) //stream end indication
          {
            ack_ok = 1;
            //Serial.println(F("Conn end ack"));
#if TTC_INTERNAL_LOGS
            logln(F("[sunr] ack"));
#endif
            break;
          }
          else if ((dec_c == '*') && (last_in == '\n')) // \n*ERROR
            error = 1;
          else
          {
            parse_sunrise(dec_c);
            if(error) log(dec_c);
            //Serial.print(dec_c);
          }
          last_in = dec_c;
        }
      }
    }
    TCPclient->stop();
    TCPclient->flush();
    //Serial.println(F("Conn stop."));

    if (!ack_ok)
    {
		
#if TTC_INTERNAL_LOGS
		logln(F("[sunr] ack failed"));
#endif
	  RTC_synced = 0;
	}

    return w_count;
  }

  return ERR_FAILED_CONN;
}

void TTC::weather_parsing_reset(void) //reset weather parser
{
  w_flag = 0;
  w_count = 0;
  w_accum = "";
}




void TTC::parse_sunrise(char c)
{
  if (w_flag == 0)
  {
    if (c == '{')
      w_flag = 1;
  }
  else if (w_flag == 1)
  {
    if (c == '[')
    {
      w_flag = 2;
      w_comma = 0;
      w_accum = "";
      sunrise_hour = 255;	//sunrise hour
      sunrise_minutes = 255;	//sunrise minutes
      noon_hour = 255;	//noon
      noon_minutes = 255;
      sunset_hour = 255;	//sunset
      sunset_minutes = 255;
      noon_angle = -1;
      moon_phase = -1;
    }
    else if (c == '}')
      w_flag = 0;
  }
  else if (w_flag == 2)
  {
    if ((c == ',') || (c == ']'))
    {
      if (w_accum.length() > 0)
      {
        if (w_comma == 1)
          sunrise_hour = w_accum.toInt();
        else if (w_comma == 2)
          sunrise_minutes = w_accum.toInt();
        else if (w_comma == 3)
          noon_hour = w_accum.toInt();
        else if (w_comma == 4)
          noon_minutes = w_accum.toInt();
        else if (w_comma == 5)
          sunset_hour = w_accum.toInt();
        else if (w_comma == 6)
          sunset_minutes = w_accum.toInt();

        else if (w_comma == 7)
          noon_angle = w_accum.toFloat();
        else if (w_comma == 8)
          moon_phase = w_accum.toFloat();
      }
      w_accum = "";

      if (c == ']')
      {
        if (sunrise_hour != 255)
        {
          w_count = 1;
        }
        w_flag = 1;
      }
      else
        w_comma++;
    }
    else
      w_accum += c;
  }

  /*
    {[0,6,26,12,10,17,56,34.497128794,23.69387964]}
//day,sr_h,sr_m,sn_h,sn_m,ss_h,ss_m,se,mp
  */
}


#if defined(WEATHER_HOURS_MAX) && (WEATHER_HOURS_MAX > 0)

int TTC::fetchWeather(TCPClientObj *TCPclient, String geo_lat, String geo_lon, int forecast_hours)
{
  if (!node_token_valid)
    return ERR_NO_NODE_TOKEN;

#if TTC_INTERNAL_LOGS
  log(F("[wthr] Connecting\t"));
#endif
  //WiFiClient TCPclient;
  if (!TCPclient->connect(_TTC_server, TT_TCP_port))
  {
    TCP_fails++;
#if TTC_INTERNAL_LOGS
    log(F("Failed\t"));
    logln(TCP_fails);
#endif
    if (TCP_fails >= 10)
      return ERR_FAILED_CONN_10x;
    return ERR_FAILED_CONN;
  }
  else
  {
    TCP_fails = 0;
    logln(F("OK"));

    unsigned long tcp_start_time = millis();

    String custom_request = "{\"weather\":{\"lat\":";
    custom_request += geo_lat;
    custom_request += ",\"lon\":";
    custom_request += geo_lon;
    custom_request += ",\"hours\":";
    custom_request += String(forecast_hours);
    custom_request += "}}";

    //Serial.println(custom_request);

    Start_AUTH(TCPclient);

    //Now send the rest of the request
    // use 'TCP_send_enc' to send Strings. Use TCPclient.write(enc_char(' ')) to send single characters
    TCP_send_enc(TCPclient, custom_request);

    TCPclient->write(enc_char('\n')); //"write" only sends single chars, cannot use print
    TCPclient->write(enc_char('\n')); //must send double new line as a packet end indicator
    //Serial.println(F("Conn req sent"));

    weather_parsing_reset();
    bool dec_started = 0;
    bool ack_ok = 0;
    bool error = 0;
    char last_in = '\0';

    while ((millis() - tcp_start_time) < TTC_connection_timeout) //timeout
    {
      if (TCPclient->available())
      {
        char ch = static_cast<char>(TCPclient->read());
        if (!dec_started)
        {
          if ((last_in == '\b') && (ch == '\r')) //decryption start indication
          {
            dec_start();
            dec_started = 1;
            //Serial.print("\n[D]\n");
          }
          last_in = ch;
        }
        else
        {
          char dec_c = dec_char(ch);

          if ((last_in == '\n') && (dec_c == '\b')) //stream end indication
          {
            ack_ok = 1;
            //Serial.println(F("Conn end ack"));
#if TTC_INTERNAL_LOGS
            logln(F("[wthr] ack"));
#endif
            break;
          }
          else if ((dec_c == '*') && (last_in == '\n')) // \n*ERROR
            error = 1;
          else
          {
            parse_weather(dec_c);
            if(error) log(dec_c);
            //Serial.print(dec_c);
          }
          last_in = dec_c;
        }
      }
    }
    TCPclient->stop();
    TCPclient->flush();
    //Serial.println(F("Conn stop."));

    if (!ack_ok)
    {
		
#if TTC_INTERNAL_LOGS
		logln(F("[wthr] ack failed"));
#endif
	  RTC_synced = 0;
	}

    return w_count;
  }

  return ERR_FAILED_CONN;
}

void TTC::weather_init(void)
{
  for (uint8_t i = 0; i < WEATHER_HOURS_MAX; i++)
  {
    forecast[i].temp = INVALID_VALUE;
    forecast[i].humid = INVALID_VALUE;
    forecast[i].perc = INVALID_VALUE;
    //forecast[i].rain = INVALID_VALUE;
    //forecast[i].snow = INVALID_VALUE;
    //forecast[i].fog = INVALID_VALUE;
    forecast[i].symbol = "";
  }
}


void TTC::parse_weather(char c)
{
  if (w_flag == 0)
  {
    if (c == '{')
      w_flag = 1;
  }
  else if (w_flag == 1)
  {
    if (c == '[')
    {
      w_flag = 2;
      w_comma = 0;
      w_accum = "";
      p_hr = INVALID_VALUE;
      p_temp = INVALID_VALUE;
      p_humd = INVALID_VALUE;
      p_perp = INVALID_VALUE;
      w_symbol = "";
      //p_snow = INVALID_VALUE;
      //p_rain = INVALID_VALUE;
      //p_fog = INVALID_VALUE;
    }
    else if (c == '}')
      w_flag = 0;
  }
  else if (w_flag == 2)
  {
    if ((c == ',') || (c == ']'))
    {
      if (w_accum.length() > 0)
      {
        if (w_comma == 0)
          p_hr = w_accum.toInt();
        else if (w_comma == 1)
          p_temp = w_accum.toFloat();
        else if (w_comma == 2)
          p_humd = w_accum.toFloat();
        else if (w_comma == 3)
          p_perp = w_accum.toFloat();
        else if (w_comma == 4)
        {
          w_symbol = w_accum;
          //match_weather_severity();
        }
      }
      w_accum = "";

      if (c == ']')
      {
        if ((p_hr >= 0) && (p_hr < WEATHER_HOURS_MAX) && (p_hr >= 0) && (p_hr != INVALID_VALUE) && (p_temp != INVALID_VALUE) && (p_humd != INVALID_VALUE) && (p_perp != INVALID_VALUE))
        {
          forecast[p_hr].temp = p_temp;
          forecast[p_hr].humid = p_humd;
          if (p_perp >= 0)
            forecast[p_hr].perc = (p_perp * 100);
          else
            forecast[p_hr].perc = 0;
          forecast[p_hr].symbol = w_symbol;
          //forecast[p_hr].snow = p_snow;
          //forecast[p_hr].rain = p_rain;
          //forecast[p_hr].fog = p_fog;

          w_count++;
        }
        w_flag = 1;
      }
      else
        w_comma++;
    }
    else
      w_accum += c;
  }

  /*
    {[0,24,83.2,-1,partlycloudy_day],
    [1,24,82.9,-1,partlycloudy_day],
    [2,24.1,82.9,-1,partlycloudy_day]}
  */
}

/*
void TTC::match_weather_severity(void)
{
  //You can add more symbols from this list:
  //https://api.met.no/weatherapi/weathericon/2.0/documentation#List_of_symbols
  //Symbols don't have to include full word... e.g., if symbols says 'rainthunder', it will detect 'rain' from the list below
  const SymbolClass weather_severity[TOTAL_SYMBOLS] = {

      {"lightsleetshowers", 50, 50, 30}, //symbol, snow melt load factor, rain, fog
      {"sleetshowers", 50, 80, 50},
      {"heavysleetshowers", 50, 100, 80},
      {"lightssleetshowersandthunder", 50, 50, 20},
      {"sleetshowersandthunder", 50, 50, 60},
      {"heavysleetshowersandthunder", 50, 50, 80},
      {"lightsnowshowers", 50, 50, 20},
      {"snowshowers", 50, 60, 40},
      {"lightssnowshowersandthunder", 50, 50, 30},
      {"snowshowersandthunder", 50, 50, 50},
      {"lightsleet", 50, 50, 10},
      {"sleet", 50, 70, 20},
      {"heavysleet", 50, 100, 60},
      {"lightsleetandthunder", 50, 50, 20},
      {"sleetandthunder", 50, 50, 30},
      {"heavysleetandthunder", 50, 50, 60},
      {"lightsnow", 50, 50, 20},
      {"snow", 50, 50, 40},
      {"lightsnowandthunder", 50, 50, 20},
      {"snowandthunder", 50, 50, 25},
      {"heavysnow", 100, 50, 60},
      {"heavysnowshowers", 100, 100, 70},
      {"heavysnowshowersandthunder", 100, 100, 80},
      {"heavysnowandthunder", 100, 100, 80}, //max 100

      {"lightrainshowers", 0, 50, 30},
      {"rainshowers", 0, 50, 50},
      {"heavyrainshowers", 0, 100, 80},
      {"lightrainshowersandthunder", 0, 60, 30},
      {"rainshowersandthunder", 0, 90, 60},
      {"heavyrainshowersandthunder", 0, 100, 70},
      {"lightrain", 0, 50, 20},
      {"rain", 0, 50, 40},
      {"heavyrain", 0, 100, 70},
      {"lightrainandthunder", 0, 50, 70},
      {"rainandthunder", 0, 80, 60},
      {"heavyrainandthunder", 0, 100, 80},

      {"clear", 0, 0, 0}, //min 0
      {"fair", 0, 0, 0},  //keep single words at end
      {"partlycloudy", 0, 0, 0},
      {"cloudy", 0, 0, 0},
      {"fog", 0, 0, 100}}; //change the snow severity level for each symbol (0-100)

  for (uint8_t s = 0; s < TOTAL_SYMBOLS; s++)
  {
    if (w_symbol.indexOf(weather_severity[s].symbol) >= 0)
    {
      p_snow = weather_severity[s].snow_index;
      p_rain = weather_severity[s].rain_index;
      p_fog = weather_severity[s].fog_index;
      return;
    }
  }
}
*/

#endif
