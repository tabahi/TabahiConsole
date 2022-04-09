
void checkWeatherForecast()
{
#if defined(WEATHER_HOURS_MAX) && (WEATHER_HOURS_MAX>0)
  //Should define at top of TabahiWeather.h :
  //#define WEATHER_HOURS_MAX 12
  //Then pass parameters as:
  String geo_lat = "60.362";
  String geo_lon = "5.013";
  int forecast_hours = WEATHER_HOURS_MAX;

  WiFiClient TCPclient;
  uint8_t hours_reported = Console.fetchWeather(&TCPclient, Console.get_String(NODE_TOKEN), geo_lat, geo_lon, forecast_hours);
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
      Serial.print(Console.forecast[i].rain);
      Serial.print('\t');
      Serial.print(Console.forecast[i].snow);
      Serial.print('\t');
      Serial.println(Console.forecast[i].fog);
    }
  }

#endif
}
