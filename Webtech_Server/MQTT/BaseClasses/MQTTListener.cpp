/*
Copyright (c) 2023-2023 Webtech Projekt
*/

#include "MQTTListener.hpp"

///////////////////////////////////////////////////////////////////////////////////
// Callback Klasse

/**
 * Wird aufgerufen, wenn eine MQTT-Nachricht eintrifft.
 */
void ListenerCallback::message_arrived(mqtt::const_message_ptr msg)
{
    listener_->message_arrived(msg);
}

/**
 * Wird aufgerufen, wenn das Senden einer MQTT-Nachricht fehlschlägt.
 */
void ListenerCallback::on_failure(const mqtt::token& tok)
{
    listener_->message_failed(tok);
}

/**
 * Wird aufgerufen, wenn das Senden einer MQTT-Nachricht erfolgreich war.
 */
void ListenerCallback::on_success(const mqtt::token& tok)
{
    listener_->message_success(tok);
}

///////////////////////////////////////////////////////////////////////////////////
// MQTT Listener Klasse

/**
 * Konstruktor für den MQTTListener.
 * Initialisiert den MQTT-Client mit dem gegebenen Broker und Topic.
 */
MQTTListener::MQTTListener(const std::string& broker, const std::string& topic)
    : broker_(broker), topic_(topic), connected_(false), client_(broker, "") {}

/**
 * Destruktor für den MQTTListener.
 * Stellt sicher, dass eine bestehende Verbindung beim Zerstören des Objekts getrennt wird.
 */
MQTTListener::~MQTTListener()
{
    if (connected_) 
    {
        disconnect();
    }
}

/**
 * Stellt eine Verbindung zum MQTT-Broker her.
 */
void MQTTListener::connect()
{
    mqtt::connect_options connOpts;
    connOpts.set_keep_alive_interval(20);
    connOpts.set_clean_session(true);

    try 
    {
        client_.connect(connOpts)->wait();
        connected_ = true;
    }
    catch (const mqtt::exception& exc)
    {
        std::cerr << "Error: Unable to connect to the MQTT broker: " << exc.what() << std::endl;
    }
}

/**
 * Abonniert das vorher festgelegte Topic beim MQTT-Broker.
 */
void MQTTListener::subscribe()
{
    try 
    {
        client_.subscribe(topic_, 0)->wait();
    }
    catch (const mqtt::exception& exc) 
    {
        std::cerr << "Error: Unable to subscribe to topic '" << topic_ << "': " << exc.what() << std::endl;
    }
}

/**
 * Trennt die Verbindung zum MQTT-Broker.
 */
void MQTTListener::disconnect()
{
    // Stop Listening
    stopProcessing();

    try 
    {
        client_.disconnect()->wait();
        connected_ = false;
    }
    catch (const mqtt::exception& exc) 
    {
        std::cerr << "Error: Unable to disconnect from the MQTT broker: " << exc.what() << std::endl;
    }
}

/**
 * Startet den Prozess, um auf eingehende MQTT-Nachrichten zu hören.
 */
void MQTTListener::processMessages()
{
    ListenerCallback cb(this);
    client_.set_callback(cb);

    stopProcessing_ = false;

    try
    {
        client_.start_consuming();  // Startet das asynchrone Konsumieren

        while (!stopProcessing_)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        client_.stop_consuming();  // Stoppt das asynchrone Konsumieren
    }
    catch (const mqtt::exception& exc)
    {
        std::cerr << "Error: MQTT error occurred: " << exc.what() << std::endl;
    }

    std::cerr << "Shutdown Completed for MQTTListener" << std::endl;
}

/**
 * Stoppt das Hören auf eingehende MQTT-Nachrichten.
 */
void MQTTListener::stopProcessing() 
{
    stopProcessing_ = true;
}
