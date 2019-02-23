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

void MqttClient::setTLSOptions(const char *serverFingerprint) {
  PRINT("MQTT: SSL options set to ");

  if ((serverFingerprint == NULL) || (strcmp(serverFingerprint, "") == 0)) {
    // Don’t verify any X509 certificates
    this->wifiClientSSL.setInsecure();
    PRINTLN("INSECURE connection.");
  } else {
    this->wifiClientSSL.setFingerprint(serverFingerprint);
    PRINT("Fingerprint '");
    PRINT(serverFingerprint);
    PRINTLN("'.");
  }
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
  this->setTLSOptions(serverFingerprint);
  this->serverAddress  = serverAddress;
  this->serverPort     = serverPort;
  this->serverUsername = serverUsername;
  this->serverPassword = serverPassword;
  this->topics         = new String(topic);
  this->topicsCount    = 1;
  this->deviceName     = deviceName;

  this->pubSubClient.setServer(serverAddress, serverPort);
  this->pubSubClient.setClient(this->wifiClientSSL);
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
  this->setTLSOptions(serverFingerprint);
  this->serverAddress  = serverAddress;
  this->serverPort     = serverPort;
  this->serverUsername = serverUsername;
  this->serverPassword = serverPassword;

  this->topics      = topics;
  this->topicsCount = topicsCount;
  this->deviceName  = deviceName;

  this->pubSubClient.setServer(serverAddress, serverPort);
  this->pubSubClient.setClient(this->wifiClientSSL);
  this->pubSubClient.setCallback(callback);
}

void MqttClient::reconnect() {
  if (!pubSubClient.connected() && ((millis() - lastFailedConnectionAttempt) >= 5 * 1000)) {
    // while (!pubSubClient.connected()) { // Loop until we're reconnected
    // Use clean session if the connection is lost for more than 10 sec
    bool useCleanSession = lastMqttConnectedMillis == 0 ||
                           (millis() - lastMqttConnectedMillis) > 10 * 1000;

    PRINT("MQTT: Attempting connection to ");
    PRINT(serverAddress);
    PRINT(":");
    PRINT(serverPort);

    if (useCleanSession) {
      PRINTLN(". Clean session is used.");
    } else {
      PRINTLN(". Preseve the session.");
    }
    PRINT("MQTT: Connection to ");
    PRINT(serverAddress);
    PRINT(":");
    PRINT(serverPort);

    if (pubSubClient.connect(deviceName, serverUsername, serverPassword,
                             useCleanSession)) {
      PRINTLN(" is successful.");
      lastMqttConnectedMillis     = millis();
      lastFailedConnectionAttempt = 0;

      // Once connected, resubscribe to all topics
      for (unsigned int i = 0; i < this->topicsCount; i++) {
        this->subscribe(this->topics[i].c_str());
      }
    } else {
      char  *sslErrorDesc    = NULL;
      size_t sslErrorDescLen = 0;
      int    sslErrorCode    = this->wifiClientSSL.getLastSSLError(sslErrorDesc, sslErrorDescLen);

      // Convert the description to string
      char sslErrorDescC[sslErrorDescLen + 1];
      memcpy(sslErrorDescC, sslErrorDesc, sslErrorDescLen);
      sslErrorDescC[sslErrorDescLen] = '\0';
      String sslErrorDescStr = String(sslErrorDescC);

      PRINT(" failed, MQTT rc: ");
      PRINT(pubSubClient.state());
      PRINT(" , SSL rc: ");
      PRINT(sslErrorCode);
      PRINT(", error: ");
      PRINT(sslErrorDescStr);
      PRINTLN(". Next attempt in 5 seconds.");

      // delay(5000);
      lastFailedConnectionAttempt = millis();
    }
  }
}

void MqttClient::loop() {
  if (!this->pubSubClient.connected()) {
    reconnect();
  }
  pubSubClient.loop();
}

void MqttClient::publish(String topic, String msg, bool retained) {
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
