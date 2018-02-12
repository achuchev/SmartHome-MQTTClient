#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

class MqttClient {
public:

  MqttClient();
  MqttClient(const char *serverAddress,
             uint16_t serverPort,
             const char *deviceName,
             const char *serverUsername,
             const char *serverPassword,
             const char *topic,
             const char *serverFingerprint,
             std::function<void(char *, uint8_t *, unsigned int)>callback);

  MqttClient(const char *serverAddress,
             uint16_t serverPort,
             const char *deviceName,
             const char *serverUsername,
             const char *serverPassword,
             String topics[],
             size_t topicsCount,
             const char *serverFingerprint,
             std::function<void(char *, uint8_t *, unsigned int)>callback);

  void publish(String topic,
               String msg);
  void subscribe(const char *topic);
  void loop();

private:

  WiFiClientSecure wifiClientSSL;
  PubSubClient pubSubClient;
  const char *serverAddress;
  uint16_t serverPort;
  const char *serverUsername;
  const char *serverPassword;
  const char *serverFingerprint;
  String *topics;
  size_t topicsCount;
  const char *deviceName;
  unsigned long lastMqttConnectedMillis = 0;
  void reconnect();
  void verifyServer();
};
#endif // ifndef MQTT_CLIENT_H
