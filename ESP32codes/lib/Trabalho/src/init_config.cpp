
void init_sensor_config(char* json_config){
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, json_config);

  if (error) {
    Serial.print("Erro ao parsear JSON: ");
    Serial.println(error.c_str());
    return;
  }

  
}