// biblkiotecas 
//Mensagem só podem ser reconhecidas se tiver o endereço foi utilizado esse método para segurança das informações transmitidas 


#include "heltec.h"
//#include <WiFi.h>
#include <FirebaseESP32.h>
#include <HTTPClient.h>
#include <DNSServer.h> 
#include <WebServer.h>
#include <WiFiManager.h>
 
WiFiManager wifiManager;//Objeto de manipulação do wi-fi

//#define Led  12
//#define botao  13
#define BAND    433E6  // Frequência  da rede Lora
#define FIREBASE_HOST "iotdom.firebaseio.com" // http://  https:// link firebase
#define FIREBASE_AUTH "dYizAfE0FSL5mIwo9tlTysHkLhqD6D9uKmXsxRkx" // chave de autenticacao


//Define FirebaseESP32 data object
FirebaseData firebaseData;

void printResult(FirebaseData &data);

String path = "/CASA";
int Estado_Relay = 0;
int ON = 1;
int OFF = 0;
String RELAYON = "RELAYON";   
String RELAYOFF = "RELAYOFF";   

int btn1 = 34;
//Pinos que são ligados no módulo de relés
int Led = 12;

void printResult(FirebaseData &data);

String outgoing;              

byte gatewayEndereco = 0xBB;     
byte end_Sensor_SCT013 = 0xFD; //0xFD SENSOR_SCT013     
byte end_Relay = 0xFA; // Raly_1

byte msgCount = 0;            
long lastSendTime = 0;        
int interval = 1000;          

void setup()
{
    Serial.begin(115200);
    pinMode(btn1, INPUT);
    pinMode(Led, OUTPUT);
    
  //callback para quando entra em modo de configuração AP
  wifiManager.setAPCallback(configModeCallback); 
  //callback para quando se conecta em uma rede, ou seja, quando passa a trabalhar em modo estação
  wifiManager.setSaveConfigCallback(saveConfigCallback); 
  while (WiFi.status() != WL_CONNECTED)
  {
    if(WiFi.status()== WL_CONNECTED){ //Se conectado na rede
      digitalWrite(Led,HIGH); //Acende LED Vermelho
      Serial.println("LED ON");
   }
   else{ //se não conectado na rede
      digitalWrite(Led,LOW); //Apaga LED Vermelho LOW
      Serial.println("LED OFF");
      wifiManager.autoConnect();//Função para se autoconectar na rede
   }
  }

  //WIFI Kit series V1 not support Vext control
  Heltec.begin(false /*DisplayEnable Enable*/, true /*Heltec.LoRa Enable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);

  Serial.println("Heltec.LoRa Duplex");

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  Firebase.setwriteSizeLimit(firebaseData, "tiny");

  //Callback quando tem alguma alteração no firebase
  if (Firebase.setInt(firebaseData, path + "/relay/", 0)) {
    printResult;
  }
  if (Firebase.setString(firebaseData, path + "/SCT013/", "")) {
    printResult;
  }

}

void loop()
{
  if (millis() - lastSendTime > interval)
  {
    if (digitalRead(btn1) == HIGH) //Caso o botão 1 foi pressionado
    {
      Serial.println("Abertura Portal"); //Abre o portal
      digitalWrite(Led,HIGH); //Acende LED
     
      wifiManager.resetSettings();       //Apaga rede salva anteriormente
      if(!wifiManager.startConfigPortal("ESP32-CONFIG", "12345678") ){ //Nome da Rede e Senha gerada pela ESP
        Serial.println("Falha ao conectar"); //Se caso não conectar na rede mostra mensagem de falha
        delay(2000);
        ESP.restart(); //Reinicia ESP após não conseguir conexão na rede
      }
      else{       //Se caso conectar 
        Serial.println("Conectado na Rede!!!");
        ESP.restart(); //Reinicia ESP após conseguir conexão na rede 
      }
   }
    
//   String RELAYON = "RELAYON";   
//   String RELAYOFF = "RELAYOFF";   
    if (Firebase.getInt(firebaseData, path + "/relay" )) {
      printResult(firebaseData);
      Serial.println("Estado_Relay");
      Serial.println(Firebase.getInt(firebaseData, path + "/relay" ));
      
      if (Estado_Relay == ON) {
        Serial.println("Estado_Relay: ON");
        sendMessage(RELAYON);
      } else
       if (Estado_Relay == OFF) {
          Serial.println("Estado_Relay: OFF");
          sendMessage(RELAYOFF);
        }
     }   
    lastSendTime = millis();            
    interval = random(1000) + 2000;    
}
  //Parse for a packet, and call onReceive with the result:
  onReceive(LoRa.parsePacket());
}

void sendMessage(String outgoing)
{
  LoRa.beginPacket();                   // Inicia Pacote
  LoRa.write(end_Sensor_SCT013);        // add endereço de destino
  LoRa.write(gatewayEndereco);          // add Endereço do remetente
  LoRa.write(msgCount);                 // add mensagem ID
  LoRa.write(outgoing.length());        // add comprimento do pacote
  LoRa.print(outgoing);                 // add pacote
  LoRa.endPacket();                     // terminar o pacote e envia
  msgCount++;                           
}

void onReceive(int packetSize)
{
  if (packetSize == 0) return;          

  // read packet header bytes:
  int endRecebido = LoRa.read();       // Endereço do destinatário
  byte remetente = LoRa.read();        // Endereço do remetente
  byte entradaMsgId = LoRa.read();     // entrada msg ID
  byte entradaLength = LoRa.read();    // entrada msg comprimento

  String entrada = "";

  while (LoRa.available())
  {
    entrada += (char)LoRa.read();
  }

  if (entradaLength != entrada.length())
  { // check length for error
    Serial.println("erro: o comprimento da mensagem não corresponde");
    return;                             // pular o restante da função
  }

  // se o destinatário não for este dispositivo
  if (endRecebido != gatewayEndereco && endRecebido != 0xFF) {
    Serial.println("Esta mensagem não é para mim.");
    return;                             // pular o restante da função
  }



  if (endRecebido == gatewayEndereco && remetente == 0xFD) { //se a menssangem e para este sensor 0xFD SENSOR SCT013
    Serial.println();
    Serial.println("Messagem: 0xFD sensor " + entrada);
    if (Firebase.setString(firebaseData, path + "/SCT013/", entrada)) {
      printResult;
    }

    Serial.println();
    
  }

}

void printResult(FirebaseData &data)
{
  if (data.dataType() == "int")
  {
    Serial.println("printResult");
    Serial.println(data.intData());
    Estado_Relay = data.intData();    
  }
}


//callback que indica que o ESP entrou no modo AP
void configModeCallback (WiFiManager *myWiFiManager) {  
  Serial.println("Entrou no modo de configuração");
  Serial.println(WiFi.softAPIP()); //imprime o IP do AP
  Serial.println(myWiFiManager->getConfigPortalSSID()); //imprime o SSID criado da rede
}
 
//Callback que indica que salvamos uma nova rede para se conectar (modo estação)
void saveConfigCallback () {
  Serial.println("Configuração salva");
}