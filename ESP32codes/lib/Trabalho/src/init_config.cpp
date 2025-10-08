
void init_sensor_config(char* json_config){
    //utiliza 200 bytes, pode utilizar mais se for necessário
    StaticJsonDocument<200> doc = json_config;

    DeserializationError error = deserializeJson(doc, json);

    if (error) {
    Serial.print("Erro ao parsear JSON: ");
    Serial.println(error.c_str());
    return;
  }

  /*
   Setar as variáveis pelos campos;
   int pin1 = doc["pin1"]
   sendo
   exemplo:
   {
    "pin1" : 0,
    "pin2" : 3
    ...
   }

   Verificação de erros, se um pino não pode ser utilizado para algo
  */
}