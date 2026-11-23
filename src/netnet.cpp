#include <Arduino.h>
#include <WiFi.h>
#include "netnet.h"
#include "senha.h"

const unsigned long tempoConexao = 10000;
const unsigned long tempoReConexao = 5000;

void conexaoWifi() {
  Serial.printf("Conectando ao Wi-Fi: %s\n", SSID);

  WiFi.begin(SSID, SENHA);

  unsigned long tempoWifi = millis();

  while (WiFi.status() != WL_CONNECTED &&
         millis() - tempoWifi < tempoConexao) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi conectado com sucesso!");
    Serial.print("Endereco IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFalha ao se conectar.");
  }
}

void reConexaoWifi() {
  static unsigned long tempoUltimaConexao = 0;

  unsigned long tempoAtual = millis();

  if (tempoAtual - tempoUltimaConexao >= tempoReConexao) {

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Conexao Wi-Fi perdida!");
      conexaoWifi();
    }

    tempoUltimaConexao = millis();
  }
}