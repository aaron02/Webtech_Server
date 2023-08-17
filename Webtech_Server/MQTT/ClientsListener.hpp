/*
Copyright (c) 2023-2023 Webtech Projekt
*/

#pragma once

#include "../../Webtech_Server.h"
#include "BaseClasses/MQTTListener.hpp"

class MQTTListener;

///////////////////////////////////////////////////////////////////////////////////
// ClientsListener
/**
 * Die ClientsListener-Klasse erbt von MQTTListener und implementiert
 * spezifische Funktionen f�r das Zerlegen und Auslesen von MQTT-Topics
 * unter Verwendung der JSON-Bibliothek.
 */
class ClientsListener : public MQTTListener
{
public:
    /**
     * Konstruktor f�r die ClientsListener-Klasse.
     *
     * @param broker Der Broker-Endpunkt f�r den MQTT-Client.
     * @param topic Das zu abonnierende MQTT-Topic.
     */
    ClientsListener(const std::string& broker, const std::string& topic)
        : MQTTListener(broker, topic) {}

    /**
     * �berschreibt die Methode message_arrived von MQTTListener.
     * Diese Methode wird aufgerufen, wenn eine MQTT-Nachricht eintrifft.
     *
     * @param msg Ein Zeiger auf die eingetroffene MQTT-Nachricht.
     */
    void message_arrived(mqtt::const_message_ptr msg) override;
};
