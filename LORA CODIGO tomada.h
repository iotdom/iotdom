//biblioteca

#include "heltec.h"
#include "EmonLib.h"
EnergyMonitor SCT013;
#define ADC_BITS    12
#define ADC_COUNTS  (1<<ADC_BITS)
#define BAND 433E6

int relay_1 = 12;




int pinSCT = 39;   //Pino analógico conectado ao SCT-013
double Potencia;
int count = 0;
double Irms;
int tensao = 127;


String outgoing;              // mensagem de saída
byte relayAddress = 0xFD;     // relay endereco
byte destination = 0xBB;      // destino de envio
byte irmsAddress = 0xFA;     // endereco IRMS



byte msgCount = 0;            // contagem de mensagens enviadas
long lastSendTime = 0;        // ultimo envio
int interval = 1000;          // intervalo de envio

void setup()

{


  SCT013.current(pinSCT, 6.0606); //calculo do sensor esta no trelo
  pinMode(relay_1, OUTPUT); // pino relay
  digitalWrite(relay_1, LOW); //sempre inicia relay em off para nao correr risco de conduzir tensao sem que usuario solicite
  //WIFI Kit series V1 not support Vext control
  Heltec.begin(false /*DisplayEnable Enable*/, true /*Heltec.LoRa Enable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  Serial.println("Heltec.LoRa Duplex");
}

void loop()
{
  if (millis() - lastSendTime > interval)
  {
    double Irms = SCT013.calcIrms(1480);
    String message = "";   // send a message
    sendMessageIrms(message + Irms); //0xFA float sensor de corrente
    sendMessageRalay(message + relay_1); //0xFD sendMessageRalay estado

    Serial.println("Sending message" + message);
    Serial.println("  ");
    Serial.println(Irms);
    lastSendTime = millis();            // timestamp the message
    interval = random(1000) + 1000;    // 2-3 seconds




  }

  // analise um pacote e chame onReceive com o resultado:
  onReceive(LoRa.parsePacket());
}

void sendMessageRalay(String outgoing)
{
  LoRa.beginPacket();                   // pacote inicial
  LoRa.write(destination);              // adiciona endereço de destino
  LoRa.write(relayAddress);             // adiciona endereço do remetente
  LoRa.write(msgCount);                 // adicionar ID a mensagem
  LoRa.write(outgoing.length());        // adiciona comprimeto ao pacote
  LoRa.print(outgoing);                 // adiciona tamanho do pacote
  LoRa.endPacket();                     // termine o pacote e envia
  msgCount++;                           // incrementa o ID da mensagem
}

void sendMessageIrms(String outgoing)
{
  LoRa.beginPacket();                  // pacote inicial
  LoRa.write(destination);              // adiciona endereço de destino
  LoRa.write(irmsAddress);             // adiciona endereço do remetente
  LoRa.write(msgCount);                 // adicionar ID a mensagem
  LoRa.write(outgoing.length());        // adiciona comprimeto ao pacote
  LoRa.print(outgoing);                 // adiciona tamanho do pacote
  LoRa.endPacket();                     // termine o pacote e envia
  msgCount++;                           // incrementa o ID da mensagem
}

void onReceive(int packetSize)
{
  String RELAYON = "RELAYON";
  String RELAYOFF = "RELAYOFF";
  if (packetSize == 0) return;          // se não houver pacote, retorne

  // ler bytes do cabeçalho do pacote:
  int recipient = LoRa.read();          // Endereço do destinatário
  byte sender = LoRa.read();            // Endereço do remetente
  byte incomingMsgId = LoRa.read();     // ID de mensagem recebida
  byte incomingLength = LoRa.read();    // Comprimento da mensagem recebida

  String incoming = "";

  while (LoRa.available())
  {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length())
  { // verifique o comprimento do pacote em busca de erro
    Serial.println("error: message length does not match length");
    return;                             // pular o resto da função
  }


  if (recipient != relayAddress && recipient != 0xFF) {  // se o destinatário não for este dispositivo
    Serial.println("This message is not for me.");
    return;                             // pular o resto da função
  }
  if (recipient == relayAddress && sender == 0xBB) { //se a menssagem for para esse dispositivo
    Serial.println("FUNCAO RELAY   " + incoming);
    if (RELAYON == incoming) {
      Serial.println("RELAY LIGADO"); // se as condicioes em cima estiver correto liga o relay
      digitalWrite(relay_1, HIGH);
    } else if (RELAYOFF == incoming) {
      Serial.println("RELAY DESLIGADO"); // se as condicioes em cima estiver correto desliga o relay
      digitalWrite(relay_1, LOW);
    }

  }

  // se a mensagem for para este dispositivo imprima os detalhes:
  Serial.println();
}