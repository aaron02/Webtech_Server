/*
Copyright (c) 2023-2023 Webtech Projekt
*/

#pragma once

#include "../../Webtech_Server.h"
#include "../../MySQL/MySQLConnection.hpp"

class MQTTListener;

/**
 * Callback-Klasse, die von den mqtt::callback- und mqtt::iaction_listener-Interfaces erbt.
 * Diese Klasse implementiert Callback-Funktionen, die von der MQTT-Bibliothek aufgerufen werden.
 */
class ListenerCallback : public virtual mqtt::callback, public virtual mqtt::iaction_listener
{
public:
    /**
     * Konstruktor, der einen MQTTListener als Parameter annimmt.
     * @param listener Zeiger auf den zugeh�rigen MQTTListener.
     */
    ListenerCallback(MQTTListener* listener) : listener_(listener) {}

    // �berschriebene Methoden aus mqtt::callback und mqtt::iaction_listener
    void message_arrived(mqtt::const_message_ptr msg) override;
    void on_failure(const mqtt::token& tok) override;
    void on_success(const mqtt::token& tok) override;

private:
    MQTTListener* listener_; ///< Zeiger auf den zugeh�rigen MQTTListener.
};

/**
 * Klasse zur Verwaltung und zum Abh�ren von MQTT-Nachrichten.
 */
class MQTTListener
{
public:
    /**
     * Konstruktor, der Broker- und Topic-Strings als Parameter annimmt.
     * @param broker String, der den MQTT-Broker angibt.
     * @param topic String, der das abonnierte MQTT-Topic angibt.
     */
    MQTTListener(const std::string& broker, const std::string& topic);
    ~MQTTListener();

    // Verbindungsverwaltung
    void connect();                 ///< Stellt eine Verbindung zum MQTT-Broker her.
    void subscribe();               ///< Abonniert das festgelegte Topic.
    void disconnect();              ///< Trennt die Verbindung zum MQTT-Broker.

    // Nachrichtenverarbeitung
    void processMessages();         ///< Startet das Abh�ren von eingehenden MQTT-Nachrichten.
    void stopProcessing();          ///< Stoppt das Abh�ren von eingehenden MQTT-Nachrichten.

    // Methoden, die von abgeleiteten Klassen �berschrieben werden sollten
    virtual void message_arrived(mqtt::const_message_ptr msg) = 0;      ///< Wird aufgerufen, wenn eine MQTT-Nachricht eintrifft.
    virtual void message_failed(const mqtt::token& tok) { };            ///< Wird aufgerufen, wenn das Senden einer MQTT-Nachricht fehlschl�gt.
    virtual void message_success(const mqtt::token& tok) { };           ///< Wird aufgerufen, wenn das Senden einer MQTT-Nachricht erfolgreich war.


private:
    std::string broker_;            ///< MQTT-Broker-String.
    std::string topic_;             ///< Abonniertes MQTT-Topic.
    bool connected_;                ///< Status der Verbindung zum MQTT-Broker.
    mqtt::async_client client_;     ///< Asynchroner MQTT-Client.

    bool stopProcessing_;           ///< Steuerflag zum Stoppen des Abh�rens von Nachrichten.

};
