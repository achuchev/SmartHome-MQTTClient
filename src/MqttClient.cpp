#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <RemotePrint.h>
#include "MqttClient.h"

MqttClient::MqttClient() {
  this->serverAddress  = NULL;
  this->serverPort     = 0;
  this->serverUsername = NULL;
  this->serverPassword = NULL;
  this->deviceName     = NULL;
}

MqttClient::MqttClient(const char *serverAddress,
                       uint16_t serverPort,
                       const char *deviceName,
                       const char *serverUsername,
                       const char *serverPassword,
                       const char *topic,
                       const char *serverFingerprint,
                       std::function<void(char *, uint8_t *,
                                          unsigned int)>callback)
{
  this->serverAddress  = serverAddress;
  this->serverPort     = serverPort;
  this->serverUsername = serverUsername;
  this->serverPassword = serverPassword;
  this->topics         = new String(topic);
  this->topicsCount    = 1;
  this->deviceName     = deviceName;

  this->pubSubClient.setServer(serverAddress, serverPort);
  this->pubSubClient.setClient(wifiClientSSL);
  this->pubSubClient.setCallback(callback);
}

MqttClient::MqttClient(const char *serverAddress,
                       uint16_t serverPort,
                       const char *deviceName,
                       const char *serverUsername,
                       const char *serverPassword,
                       String topics[],
                       size_t topicsCount,
                       const char *serverFingerprint,
                       std::function<void(char *, uint8_t *,
                                          unsigned int)>callback)
{
  this->serverAddress  = serverAddress;
  this->serverPort     = serverPort;
  this->serverUsername = serverUsername;
  this->serverPassword = serverPassword;

  this->topics      = topics;
  this->topicsCount = topicsCount;
  this->deviceName  = deviceName;

  this->pubSubClient.setServer(serverAddress, serverPort);
  this->pubSubClient.setClient(wifiClientSSL);
  this->pubSubClient.setCallback(callback);
}

void MqttClient::reconnect() {
  // Loop until we're reconnected
  while (!pubSubClient.connected()) {
    // Use clean session if the connection is lost for more than 10 sec
    bool useCleanSession = lastMqttConnectedMillis == 0 ||
                           (millis() - lastMqttConnectedMillis) > 10 * 1000;

    PRINT("MQTT: Attempting connection to ");
    PRINT(serverAddress);

    if (useCleanSession) {
      PRINTLN(". Clean session is used.");
    } else {
      PRINTLN(". Preseve the session.");
    }
    PRINT("MQTT: Connection to ");
    PRINT(serverAddress);

    if (pubSubClient.connect(deviceName, serverUsername, serverPassword,
                             useCleanSession)) {
      PRINTLN(" is successful.");
      lastMqttConnectedMillis = millis();

      // Once connected, resubscribe to all topics
      for (int i = 0; i < this->topicsCount; i++) {
        this->subscribe(this->topics[i].c_str());
      }
    } else {
      PRINT(" failed, rc=");
      PRINT(pubSubClient.state());
      PRINTLN(" try again in 5 seconds.");
      delay(5000);
    }
  }
}

void MqttClient::loop() {
  if (!this->pubSubClient.connected()) {
    reconnect();
  }
  pubSubClient.loop();
}

void MqttClient::verifyServer() {
  WiFiClientSecure wifiClientSSL;

  // Use WiFiClientSecure class to create TLS connection
  PRINT("MQTT: Connecting to ");
  PRINTLN(serverAddress);

  if (!wifiClientSSL.connect(serverAddress, serverPort)) {
    PRINTLN("connection failed");
    return;
  }

  if (wifiClientSSL.verify(serverFingerprint, serverAddress)) {
    PRINTLN("certificate matches");
  } else {
    PRINTLN("certificate doesn't match");
  }
}

void MqttClient::publish(String topic, String msg, bool retained = true) {
  PRINT("MQTT: Publish message [");
  PRINT(topic);
  PRINT("]: ");
  PRINTLN(msg.c_str());
  bool success = pubSubClient.publish(topic.c_str(),
                                      msg.c_str(),
                                      (uint8_t)1,
                                      retained);

  if (!success) {
    PRINT("MQTT: Unable to publish to topic [");
    PRINT(topic);
    PRINT("]: ");
    PRINTLN(msg.c_str());
  }
}

void MqttClient::subscribe(const char *topic) {
  PRINT("MQTT: Subscribed to '");
  PRINT(topic);
  PRINTLN("'.");
  pubSubClient.subscribe(topic);
}
