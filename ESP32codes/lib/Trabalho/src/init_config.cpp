#include "Trabalho.hpp"
vector<Sensor> init_sensor_config(const char* json_config){
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, json_config);

  if (error) {
    Serial.print("Erro ao parsear JSON: ");
    Serial.println(error.c_str());
    return vector<Sensor>();
  }

  vector<Sensor> sensores;

  if (!doc.is<JsonArray>()) {
    Serial.println("JSON root is not an array.");
    return vector<Sensor>();
  }

  JsonArray sensores_json = doc.as<JsonArray>();
  for (JsonObject sensor_json : sensores_json) {
    Sensor sensor;
    //id
    if (sensor_json.containsKey("id") && sensor_json["id"].is<int>()) {
      sensor.id = sensor_json["id"].as<int>();
    } else {
      Serial.println("Sensor JSON missing valid 'id' field.");
      continue; // Skip this sensor if 'id' is missing or invalid
    }
    //tipo
    if (!sensor_json.containsKey("tipo") || !sensor_json["tipo"].is<int>()) {
      Serial.println("Sensor JSON missing valid 'tipo' field.");
      continue; // Skip this sensor if 'tipo' is missing or invalid
    }
    sensor.tipo = static_cast<Sensor_tipo>(sensor_json["tipo"].as<int>());
    //desc
    if (!sensor_json.containsKey("desc") || !sensor_json["desc"].is<const char*>()) {
      Serial.println("Sensor JSON missing valid 'desc' field.");
    }else
    {
      sensor.desc = sensor_json["desc"].as<string>();
    }

    //pinos
    if (!sensor_json.containsKey("pinos") || !sensor_json["pinos"].is<JsonArray>()) {
      Serial.println("Sensor JSON missing valid 'pinos' array.");
      continue; // Skip this sensor if 'pinos' is missing or invalid
    }
    JsonArray pinos_json = sensor_json["pinos"].as<JsonArray>();
    for (JsonObject pino_json : pinos_json) {
      Pino pino;
      if (!pino_json.containsKey("pino") || !pino_json["pino"].is<int>() ||
          !pino_json.containsKey("tipo") || !pino_json["tipo"].is<int>()) {
        Serial.println("Pino JSON missing valid 'pino' or 'tipo' field.");
        continue; // Skip this pin if fields are missing or invalid
      }
      pino.pin = pino_json["pino"];
      pino.tipo = static_cast<Pino_funcao>(pino_json["tipo"].as<int>());
      sensor.pinos.push_back(pino);
    }
    //atributos
    /*1*/
    if (!sensor_json.containsKey("atributo1") || !sensor_json["atributo1"].is<atributo_1_t>())
      Serial.println("Sensor JSON missing valid 'atributo1' field.");
    else
      sensor.atributo1 = sensor_json["atributo1"];    
    /*2*/
    if (!sensor_json.containsKey("atributo2") || !sensor_json["atributo2"].is<atributo_2_t>())
      Serial.println("Sensor JSON missing valid 'atributo2' field.");
    else
      sensor.atributo2 = sensor_json["atributo2"];    
    /*3*/
    if (!sensor_json.containsKey("atributo3") || !sensor_json["atributo3"].is<atributo_3_t>())
      Serial.println("Sensor JSON missing valid 'atributo3' field.");
    else 
      sensor.atributo3 = sensor_json["atributo3"];    
    /*4*/
    if (!sensor_json.containsKey("atributo4") || !sensor_json["atributo4"].is<atributo_4_t>())
      Serial.println("Sensor JSON missing valid 'atributo4' field.");
    else
      sensor.atributo4 = sensor_json["atributo4"];

    sensores.push_back(sensor);
  }
  
  return sensores;
}