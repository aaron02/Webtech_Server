/*
Copyright (c) 2023-2023 Webtech Projekt
*/

#include "Webtech_Server.h"

#include "MQTT/ClientsListener.hpp"
#include "MQTT/ConnectionListener.hpp"
#include "MySQL/MySQLConnection.hpp"

// Globale Flagge zum Beenden des Hintergrundprozesses
volatile sig_atomic_t shouldExit = 0;

// Signalbehandlungsfunktion für SIGINT (Ctrl+C)
void signalHandler(int signal)
{
    if (signal == SIGINT)
    {
        shouldExit = 1;
    }
}

int main()
{
    // Signalbehandlung für SIGINT (Ctrl+C) festlegen
    std::signal(SIGINT, signalHandler);

    // Server Address Festlegen
    // Es wird davon ausgegangen das der MQTT Server auf den selben Maschine auf Default Ports Betrieben wird
    std::string serverAddress = "localhost:1883";

    // Erstelle einen MQTTListener für einkommende Daten sowie Authentifierzierungs anfragen
    ConnectionListener  listener_connection(serverAddress, "client/accepted");
    ClientsListener     listener_clients(serverAddress, "Nodes/+/Data");

    // Connect to MQTT Broker
    listener_connection.connect();
    listener_clients.connect();

    // Warte bis die Verbindung aufgebaut ist
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Subscribe die benötigten Topics
    listener_connection.subscribe();
    listener_clients.subscribe();

    // Starten Sie den zyklischen Aufruf des Listeners in einem separaten Thread
    std::thread listenerThread_connection(&MQTTListener::processMessages, &listener_connection);
    std::thread listenerThread_clients(&MQTTListener::processMessages, &listener_clients);

    // MySQL Server Verbindung aufbauen und Initialiseren
    // Es wird davon ausgegangen das der MySQL Server auf den selben Maschine auf Default Ports Betrieben wird
    MySQLConnectionInfo _connectionInfo("tcp://127.0.0.1:3306; webtech; zbwzbw; node_server");
    sMySQL.setup(std::make_unique<MySQLConnectionInfo>(_connectionInfo));
    if (!sMySQL.connect())
    {
        shouldExit = true;
        std::cerr << "Error: MySQL Connection Failed, Shuting Down Server" << std::endl;
    }

    // Hauptloop des Programms dient zu Monitoring zwecken und Polling der Datenank
    while (!shouldExit)
    {
        ////////////////////////
        // Main Thread
        sMySQL.monitorLastSeen();

        ////////////////////////
        // Sleep 10s
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    ///////////////////////////
    // Programm Shutdown Prozedur
    std::cerr << "Shuting Down..." << std::endl;

    // Setze Alle Nodes auf Offline
    for (auto node : sMySQL.getNodeContainer())
        sMySQL.updateNodeStatusInDB(node.id, "online", false);

    // Beenden der Listener
    std::cerr << "Shutdown Startet for MQTTListener (Connections)" << std::endl;
    listener_connection.disconnect();

    std::cerr << "Shutdown Startet for MQTTListener (Clients)" << std::endl;
    listener_clients.disconnect();

    // Warten Sie auf den Listener-Thread, bis er beendet ist
    listenerThread_connection.join();
    listenerThread_clients.join();

    std::cerr << "Shutdown Completed" << std::endl;
	return 0;
}
