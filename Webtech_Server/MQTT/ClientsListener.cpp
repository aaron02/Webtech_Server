/*
Copyright (c) 2023-2023 Webtech Projekt
*/

#include "ClientsListener.hpp"

/**
 * Diese Methode wird aufgerufen, wenn eine MQTT-Nachricht eintrifft.
 *
 * @param msg Ein Zeiger auf die eingetroffene MQTT-Nachricht.
 */
void ClientsListener::message_arrived(mqtt::const_message_ptr msg)
{
    // Liest den Inhalt (Payload) der Nachricht.
    std::string payload = msg->get_payload_str();

    // Liest das Topic der Nachricht.
    std::string topic = msg->get_topic();
    std::string node_id = "unk";

    // Extrahiert die Node-ID aus dem Topic (angenommenes Format: "Nodes/{ID}/Data").
    size_t start = topic.find("Nodes/") + 6;
    size_t end = topic.find("/Data");

    // Überprüft, ob die Node-ID erfolgreich extrahiert wurde.
    if (start != std::string::npos && end != std::string::npos)
    {
        node_id = topic.substr(start, end - start);

        // Versucht, den Payload als JSON zu interpretieren.
        json data_json;
        try
        {
            data_json = nlohmann::json::parse(payload);
        }
        catch (const nlohmann::json::exception& e)
        {
            std::cerr << "Error parsing JSON: " << e.what() << std::endl;
            return;
        }

        // Gibt die empfangene ID auf der Konsole aus
        //std::cout << "Received Data from id: " << id << std::endl;

        // Befüllt eine NodeData-Struktur mit den extrahierten Werten aus dem JSON.
        NodeData data;
        data.temperature = data_json["temp"].get<float>();
        data.pressure = data_json["pres"].get<uint32_t>();
        data.altitude = data_json["alt"].get<float>();
        data.humidity = data_json["hum"].get<uint32_t>();
        data.lux = data_json["lux"].get<uint32_t>();
        data.sound = data_json["soun"].get<uint16_t>();
        data.timeStamp = data_json["time"].get<time_t>();

        // Aktualisiert die Daten des Knotens in der Datenbank und setzt seinen Online-Status.
        sMySQL.updateNodeData(node_id, data);
        sMySQL.setNodeOnline(node_id, true);
    }
}