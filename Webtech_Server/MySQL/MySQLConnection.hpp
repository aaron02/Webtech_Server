/*
Copyright (c) 2023-2023 Webtech Projekt
*/

#pragma once

#include "../../Webtech_Server.h"

// Einbinden der ben�tigten MySQL-Bibliotheken
#include <mysql_driver.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <mysql_connection.h>
#include <mysql_error.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/exception.h>

/**
 * Struktur zur Speicherung von Daten eines Knotens.
 */
struct NodeData
{
    float temperature = 0.0f;
    uint32_t pressure = 0;
    float altitude = 0.0f;
    uint32_t humidity = 0;
    uint32_t lux = 0;
    uint16_t sound = 0;
    time_t timeStamp = 0;
};

/**
 * Struktur zur Repr�sentation eines Knotens.
 */
struct Node
{
    std::string id;
    time_t lastSeen;
    bool allowed;
    bool online;

    // Last Data Received
    NodeData data;
};

/**
 * Struktur zur Speicherung von MySQL-Verbindungsinformationen.
 */
struct MySQLConnectionInfo
{
    explicit MySQLConnectionInfo(std::string const& infoString);

    std::string user;
    std::string password;
    std::string database;
    std::string host;
};

///////////////////////////////////////////////////////////////////////////////////

/**
 * Klasse zur Verwaltung der MySQL-Verbindung.
 *
 * Diese Klasse wird als Singleton implementiert, sodass sie global �ber ein Makro
 * zug�nglich ist und die MySQL-Schnittstelle verwendet werden kann.
 */
class MySQLConnection
{
private:
    MySQLConnection();
    ~MySQLConnection();

    // Verhindern von Kopieren und Verschieben des Singleton-Objekts
    MySQLConnection(MySQLConnection&&) = delete;
    MySQLConnection(MySQLConnection const&) = delete;
    void operator=(MySQLConnection&&) = delete;
    void operator=(MySQLConnection const&) = delete;

public:

    static MySQLConnection& getInstance()
    {
        static MySQLConnection instance;
        return instance;
    }

    /* MySQL Connection Infos �bernehmen */
    void setup(std::unique_ptr<MySQLConnectionInfo> connInfo) { m_connectionInfo = std::move(connInfo); }

    /* Verbindung zur Datenbank Aufbauen */
    bool connect();

    /* Polling der Audit Datenbank tabelle um �nderungen der Webseite zu aktuallisieren */
    void pollAuditTable();

    /* Alle Eintr�ge aus der Nodes Datenbank Lesen */
    bool fetchAllNodesFromDatabase();

    /* Aktuallisieren des Node Status in der Datenbank */
    void updateNodeStatusInDB(const std::string& id, const std::string& column, bool status);
    
    /* Globale Funktion zum �ndern des Online/Accepted Status */
    void setNodeStatus(std::string id, bool status, bool isOnlineUpdate, bool saveToDB = true);
    
    /* F�gt einen Node mit gegebener Id hinzu */
    void addNode(std::string id);

    /* L�scht einen Node mit gegebener Id */
    void deleteNode(std::string id);

    /* Aktuallisiert Node Daten f�r den gegebenen Node */
    void updateNodeData(std::string id, NodeData data, bool forceData = false);

    /* Setze gegebenen Node zum Status Online */
    void setNodeOnline(std::string id, bool online, bool saveToDB = true);

    /* Setze gegebenen Node zum Status Active */
    void setNodeActive(std::string id, bool active, bool saveToDB = true);

    /* Gibt True zur�ck wenn der Eintrag exestiert */
    bool isNodeInDatabase(std::string id);

    /* Gibt True zur�ck wenn der Node Allowed ist Updates in die Datenbank zu schrieben */
    bool isAllowed(const std::string& id);

    /* F�gt einen Node in den Virtuellen Container der das Abbild der Nodes Tabelle darstellt */
    bool addNodeToContainer(std::string id);
    
    /* Setze die Uhrzeit und Datum f�r den Node an dem er Updates gesendet hatt */
    void setLastSeen(std::string id);

    /* Entfernt einen Node vom Virtuellen Container */
    void removeNodeFromContainer(std::string id);

    /* �berwacht alle Nodes ob sie zu Lange keine Updates mehr gesendet haben */
    void monitorLastSeen();

    /* Getter f�r den Container */
    const std::vector<Node>& getNodeContainer() const { return mNodeContainer; }
    
private:
    std::vector<Node> mNodeContainer;
    sql::mysql::MySQL_Driver* driver_ = nullptr;
    sql::Connection* connection_ = nullptr;
    std::unique_ptr<MySQLConnectionInfo> m_connectionInfo;
};

// Makro, um den Singleton-Instance der MySQLConnection-Klasse zu erhalten.
#define sMySQL MySQLConnection::getInstance()

