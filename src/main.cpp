
#include <Arduino.h>
#include <WiFi.h>
#include <ezTime.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "netnet.h"

#define LED 2
#define DHTPIN 27
#define DHTTYPE DHT22
#define LDRPIN 32

const char *mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char *mqtt_client_id = "Diogo";
const char *mqtt_topic_sub = "bapnalin/netnet/sub";
const char *mqtt_topic_pub = "bapnalin/netnet/pub";

void connectMqtt();
void retornoMqtt(char *topic, byte *payload, unsigned int length);
void enviarSensores();

WiFiClient espClient;
PubSubClient client(espClient);

DHT dht(DHTPIN, DHTTYPE);
Timezone sp;

String msgRecebida;

bool estadoLED = false;
bool piscando = false;

unsigned long tempoPisca = 0;
unsigned long tempoSensores = 0;

const unsigned long intervaloSensores = 5000;

void setup()
{
  Serial.begin(115200);
  Serial.setTimeout(10000);

  conexaoWifi();

  if (WiFi.status() == WL_CONNECTED)
  {
    sp.setLocation("America/Sao_Paulo");
    waitForSync();
  }

  pinMode(LED, OUTPUT);
  pinMode(LDRPIN, INPUT);

  dht.begin();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(retornoMqtt);
}

void loop()
{
  reConexaoWifi();

  if (!client.connected())
  {
    connectMqtt();
  }

  client.loop();

  // ---------------------------- Enviar sensores ----------------------------

  if (millis() - tempoSensores >= intervaloSensores)
  {
    tempoSensores = millis();
    enviarSensores();
  }

  // ---------------------------- Lê a Serial ----------------------------

  if (Serial.available() > 0)
  {
    String txtdig = Serial.readStringUntil('\n');
    txtdig.trim();

    if (txtdig.length() == 0)
    {
      Serial.println("Mensagem vazia");
    }
    else
    {
      JsonDocument doc;

      time_t ts = sp.now();

      doc["disp"] = "Esp-Diogo";
      doc["msg"] = txtdig;
      doc["ts"] = ts;

      String stringJson;
      serializeJson(doc, stringJson);

      client.publish(mqtt_topic_pub, stringJson.c_str());

      Serial.print("Enviado: ");
      Serial.println(stringJson);
    }
  }

  // ---------------------------- Piscar LED ----------------------------

  if (msgRecebida == "P")
  {
    piscando = true;
    msgRecebida = "";
    tempoPisca = millis();
  }

  if (piscando)
  {
    if (millis() - tempoPisca >= 500)
    {
      tempoPisca = millis();

      estadoLED = !estadoLED;
      digitalWrite(LED, estadoLED);
    }
  }

  if (msgRecebida == "Ligar")
  {
    piscando = false;
    estadoLED = true;
    digitalWrite(LED, HIGH);
    msgRecebida = "";
  }

  if (msgRecebida == "Desligar")
  {
    piscando = false;
    estadoLED = false;
    digitalWrite(LED, LOW);
    msgRecebida = "";
  }
}

// ---------------------------- Leitura e envio dos sensores ----------------------------

void enviarSensores()
{
  float temperatura = dht.readTemperature();
  float umidade = dht.readHumidity();
  int luminosidade = analogRead(LDRPIN);

  if (isnan(temperatura) || isnan(umidade))
  {
    Serial.println("Erro ao ler o DHT");
    return;
  }

  JsonDocument doc;

  time_t ts = sp.now();

  doc["disp"] = "Esp-Diogo";
  doc["temperatura"] = temperatura;
  doc["umidade"] = umidade;
  doc["luminosidade"] = luminosidade;
  doc["ts"] = ts;

  String stringJson;
  serializeJson(doc, stringJson);

  if (client.connected())
  {
    client.publish(mqtt_topic_pub, stringJson.c_str());

    Serial.println("Dados dos sensores enviados:");
    Serial.println(stringJson);
    Serial.println("----------------------------");
  }
}

// ---------------------------- Conexão MQTT ----------------------------

void connectMqtt()
{
  while (!client.connected())
  {
    Serial.println("Conectando ao MQTT");

    if (client.connect(mqtt_client_id))
    {
      Serial.println("Conectado com sucesso ao MQTT!");
      client.subscribe(mqtt_topic_sub);
    }
    else
    {
      Serial.println("Falha na conexao com o MQTT");
      Serial.println(client.state());
      Serial.println("Tentando novamente em 5s");
      delay(5000);
    }
  }
}

// ---------------------------- Recebe MQTT ----------------------------

void retornoMqtt(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Mensagem recebida em ");
  Serial.print(topic);
  Serial.print(": ");

  String msgReceived = "";

  for (unsigned int i = 0; i < length; i++)
  {
    msgReceived += (char)payload[i];
  }

  Serial.println(msgReceived);

  JsonDocument doc;
  DeserializationError erro = deserializeJson(doc, msgReceived);

  if (erro)
  {
    Serial.print("Erro ao decodificar o Json: ");
    Serial.println(erro.c_str());
    return;
  }

  const char *dispositivo = doc["disp"] | "Desconhecido";
  const char *msg = doc["msg"] | "";

  msgRecebida = msg;

  time_t timestamp = doc["ts"] | 0;
  String data = dateTime(timestamp, "d/m/Y H:i:s");

  Serial.println("Dados decodificados:");
  Serial.print("Esp: ");
  Serial.println(dispositivo);
  Serial.print("Mensagem: ");
  Serial.println(msg);
  Serial.println(data);
  Serial.println("----------------------------");
}
