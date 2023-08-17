/*
Copyright (c) 2023-2023 Webtech Projekt
*/

#include "ConnectionListener.hpp"

/**
 * Diese Methode wird aufgerufen, wenn eine MQTT-Nachricht eintrifft.
 *
 * @param msg Ein Zeiger auf die eingetroffene MQTT-Nachricht.
 */
void ConnectionListener::message_arrived(mqtt::const_message_ptr msg)
{
    // Liest den Payload der MQTT-Nachricht
    std::string payload = msg->get_payload_str();
    std::string id = "unk";

    try
    {
        // Versucht, den Payload als JSON zu parsen
        json jsonData = json::parse(payload);

        // Überprüft, ob das JSON-Objekt ein "id"-Feld enthält
        if (jsonData.contains("id"))
        {
            // Überprüft den Datentyp des "id"-Felds und liest den Wert entsprechend aus
            if (jsonData["id"].is_string())
            {
                id = jsonData["id"];

            }
            else if (jsonData["id"].is_number())
            {
                int asNumber = jsonData["id"];
                id = std::to_string(asNumber);
            }

            // Gibt die empfangene ID auf der Konsole aus
            //std::cout << "Received id: " << id << std::endl;

            // Fügt den Knoten mit der empfangenen ID zur Datenbank hinzu und setzt seinen Status auf "online"
            sMySQL.addNode(id);
            sMySQL.setNodeOnline(id, true);

            // Setzt den Zeitstempel für den letzten Update-Vorgang
            sMySQL.setLastSeen(id);
        }
        else
        {
            // Gibt einen Fehler aus, wenn das "id"-Feld nicht im JSON gefunden wird
            std::cerr << "Error: 'id' field not found in JSON." << std::endl;
        }

    }
    // Fängt etwaige Fehler beim Parsen des JSONs ab und gibt diese aus
    catch (const json::exception& e)
    {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
    }
}