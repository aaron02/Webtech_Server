/*
Copyright (c) 2023-2023 Webtech Projekt
*/

#include "MySQLConnection.hpp"

/**
 * Parst den gegebenen String, um MySQL-Verbindungsdetails wie Host, Benutzer, Passwort und Datenbank zu extrahieren.
 *
 * @param infoString Ein durch Semikolon getrennter String mit MySQL-Verbindungsdetails.
 */
MySQLConnectionInfo::MySQLConnectionInfo(std::string const& infoString)
{
    std::istringstream iss(infoString);
    std::string part;
    int partIndex = 0;

    // Teilt die infoString an jedem Semikolon
    while (std::getline(iss, part, ';'))
    {
        // Entferne überflüssige Leerzeichen
        size_t startPos = part.find_first_not_of(" ");
        size_t endPos = part.find_last_not_of(" ");

        if (startPos != std::string::npos) 
        {
            part = part.substr(startPos, endPos - startPos + 1);
        }

        // Weise die Teile den entsprechenden Variablen zu
        switch (partIndex)
        {
            case 0: // Host
                host = part;
                break;
            case 1: // User
                user = part;
                break;
            case 2: // Password
                password = part;
                break;
            case 3: // Database
                database = part;
                break;
            default:
                // Handle unexpected part
                break;
        }
        partIndex++;
    }
}

/**
 * Standard-Konstruktor initialisiert die Mitgliedsvariablen für MySQLConnection.
 */
MySQLConnection::MySQLConnection() : 
    driver_(nullptr),
    connection_(nullptr),
    m_connectionInfo(nullptr)
{
}

/**
 * Destruktor, der für das Freigeben von zugewiesenen Ressourcen verantwortlich ist.
 */
MySQLConnection::~MySQLConnection()
{

}

/**
 * Stellt eine Verbindung zur MySQL-Datenbank mit den bereitgestellten Verbindungsinformationen her.
 *
 * @return bool Gibt true zurück, wenn die Verbindung erfolgreich ist, sonst false (für diesen Code immer true).
 */
bool MySQLConnection::connect()
{
    driver_ = sql::mysql::get_mysql_driver_instance();
    connection_ = driver_->connect(m_connectionInfo->host, m_connectionInfo->user, m_connectionInfo->password);
    connection_->setSchema(m_connectionInfo->database);

    // Lädt alle Knoten aus der Datenbank
    fetchAllNodesFromDatabase();
    return true;
}

/**
 * Fügt einen Knoten zur Datenbank hinzu oder aktualisiert ihn, wenn er bereits existiert.
 *
 * @param id ID des hinzuzufügenden oder zu aktualisierenden Knotens.
 */
void MySQLConnection::addNode(std::string id)
{
    try
    {
        // Erstelle eine SQL-Anweisung zum Hinzufügen oder Aktualisieren eines Knotens
        sql::PreparedStatement* delStmt;
        delStmt = connection_->prepareStatement("INSERT INTO nodes (id) VALUES (?) ON DUPLICATE KEY UPDATE id = ?");
        delStmt->setString(1, id);
        delStmt->setString(2, id);
        delStmt->executeUpdate();
        delete delStmt;

        addNodeToContainer(id);
    }
    catch (const sql::SQLException& e)
    {
        std::cerr << "SQL Exception in addNode: " << e.what() << std::endl;
        std::cerr << "Error Code: " << e.getErrorCode() << std::endl;
        std::cerr << "SQL State: " << e.getSQLState() << std::endl;
    }
}

/**
 * Entfernt einen Knoten aus der Datenbank.
 *
 * @param id ID des zu löschenden Knotens.
 */
void MySQLConnection::deleteNode(std::string id)
{
    // Beginne eine Transaktion
    connection_->setAutoCommit(false);

    try 
    {
        // Lösche zugehörige Daten für den Knoten
        sql::PreparedStatement* delNodeDataStmt;
        delNodeDataStmt = connection_->prepareStatement("DELETE FROM node_data WHERE id = ?");
        delNodeDataStmt->setString(1, id);
        delNodeDataStmt->executeUpdate();
        delete delNodeDataStmt;

        // Lösche den Knoteneintrag selbst
        sql::PreparedStatement* delNodeStmt;
        delNodeStmt = connection_->prepareStatement("DELETE FROM nodes WHERE id = ?");
        delNodeStmt->setString(1, id);
        delNodeStmt->executeUpdate();
        delete delNodeStmt;

        // Führe die Transaktion aus
        connection_->commit();

        removeNodeFromContainer(id);
    }
    catch (const sql::SQLException& e) 
    {
        // Bei einem Fehler, führe einen Rollback der Transaktion durch
        connection_->rollback();
    }

    // Beende die Transaktion
    connection_->setAutoCommit(true);
}

/**
 * Aktualisiert die Daten eines Knotens in der Datenbank.
 *
 * @param id ID des zu aktualisierenden Knotens.
 * @param data NodeData Struktur mit den zu aktualisierenden Daten für den Knoten.
 */
void MySQLConnection::updateNodeData(std::string id, NodeData data, bool forceData)
{   
    // Setzt den Zeitstempel für den letzten Update-Vorgang
    setLastSeen(id);

    if (isNodeInDatabase(id))
    {
        if (isAllowed(id) || forceData)
        {
            // Konvertiere time_t in ein formatierbares Zeitstempelformat
            std::tm* tm_lastSeen = std::localtime(&data.timeStamp);
            char buffer[20];
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_lastSeen);
            std::string str_lastSeen(buffer);

            try
            {
                // Aktualisiere die Daten für den Knoten in der Datenbank
                sql::PreparedStatement* updateDataStmt;
                updateDataStmt = connection_->prepareStatement("INSERT INTO node_data(id, timestamp, temperature, pressure, altitude, humidity, lux, sound) VALUES(?, ?, ?, ?, ?, ?, ?, ?) ON DUPLICATE KEY UPDATE timestamp = VALUES(timestamp), temperature = VALUES(temperature), pressure = VALUES(pressure), altitude = VALUES(altitude), humidity = VALUES(humidity), lux = VALUES(lux), sound = VALUES(sound)");
                updateDataStmt->setString(1, id);
                updateDataStmt->setString(2, str_lastSeen);
                updateDataStmt->setDouble(3, data.temperature);
                updateDataStmt->setInt(4, data.pressure);
                updateDataStmt->setInt(5, data.altitude);
                updateDataStmt->setInt(6, data.humidity);
                updateDataStmt->setInt(7, data.lux);
                updateDataStmt->setInt(8, data.sound);

                updateDataStmt->executeUpdate();
                delete updateDataStmt;
            }
            catch (const sql::SQLException& e)
            {
                std::cerr << "SQL Exception in updateNodeData: " << e.what() << std::endl;
                std::cerr << "Error Code: " << e.getErrorCode() << std::endl;
                std::cerr << "SQL State: " << e.getSQLState() << std::endl;
            }
        }
        else
        {
            std::cerr << "Error: MySQL Given Node Not Allowed to Save Data" << std::endl;
        }
    }
    else
    {
        std::cerr << "Error: MySQL Given Node Not Existant" << std::endl;
    }
}

/**
 * Aktualisiert den Status einer spezifischen Spalte für einen Knoten in der Datenbank.
 *
 * @param id ID des Knotens.
 * @param column Name der zu aktualisierenden Spalte.
 * @param status Neuer Statuswert.
 */
void MySQLConnection::updateNodeStatusInDB(const std::string& id, const std::string& column, bool status) 
{
    try
    {
        // Aktualisiere die Datenbank
        sql::PreparedStatement* stmt;
        stmt = connection_->prepareStatement("UPDATE nodes SET " + column + " = ? WHERE id = ?");
        stmt->setInt(1, status);
        stmt->setString(2, id);
        stmt->executeUpdate();
        delete stmt;
    }
    catch (const sql::SQLException& e)
    {
        std::cerr << "SQL Exception in updateNodeStatusInDB: " << e.what() << std::endl;
        std::cerr << "Error Code: " << e.getErrorCode() << std::endl;
        std::cerr << "SQL State: " << e.getSQLState() << std::endl;
    }
}

/**
 * Aktualisiert den Status eines Knotens anhand der angegebenen Parameter.
 *
 * @param id ID des Knotens.
 * @param status Neuer Statuswert.
 * @param isOnlineUpdate Flag, um zu bestimmen, ob es sich um ein Online-Update handelt.
 * @param saveToDB Flag, um zu bestimmen, ob der Status in der Datenbank gespeichert werden soll.
 */
void MySQLConnection::setNodeStatus(std::string id, bool status, bool isOnlineUpdate, bool saveToDB)
{
    // Finde den Knoten im Container
    auto it = std::find_if(mNodeContainer.begin(), mNodeContainer.end(),
        [&id](const Node& node)
        {
            return node.id == id;
        });

    if (it != mNodeContainer.end()) 
    {
        // Aktualisiere Online-Status
        if (isOnlineUpdate) 
        {
            if (it->online != status) 
            {
                it->online = status;

                std::cout << "Node with id: " << it->id << " has gone " << (status ? "Online" : "Offline") << std::endl;

                if (saveToDB)
                    updateNodeStatusInDB(id, "online", status);
            }
        }
        // Aktualisiere Erlaubnis-Status
        else 
        {
            if (it->allowed != status) 
            {
                it->allowed = status;

                std::cout << "Node with id: " << it->id << " is now " << (status ? "Allowed" : "NotAllowed") << std::endl;

                if (saveToDB)
                    updateNodeStatusInDB(id, "allowed", status);
            }
        }
    }
    else 
    {
        std::cerr << "Error: MySQL Given Node Not Existant" << std::endl;
    }
}

/**
 * Setzt den Online-Status eines Knotens in der Datenbank.
 *
 * @param id ID des Knotens.
 * @param online Boolean, der den Online-Status angibt.
 * @param saveToDB Flag, um zu bestimmen, ob der Status in der Datenbank gespeichert werden soll.
 */
void MySQLConnection::setNodeOnline(std::string id, bool online, bool saveToDB)
{
    setNodeStatus(id, online, true, saveToDB);
}

/**
 * Setzt den aktiven Status eines Knotens in der Datenbank.
 *
 * @param id ID des Knotens.
 * @param active Boolean, der den aktiven Status angibt.
 * @param saveToDB Flag, um zu bestimmen, ob der Status in der Datenbank gespeichert werden soll.
 */
void MySQLConnection::setNodeActive(std::string id, bool active, bool saveToDB)
{
    setNodeStatus(id, active, false, saveToDB);
}

/**
 * Überprüft, ob ein Knoten in der Datenbank existiert.
 *
 * @param id ID des Knotens.
 * @return bool Gibt true zurück, wenn der Knoten existiert, sonst false.
 */
bool MySQLConnection::isNodeInDatabase(std::string id)
{
    // Finde die ID im Container
    auto it = std::find_if(mNodeContainer.begin(), mNodeContainer.end(),
        [&id](const Node& node)
        {
            return node.id == id;
        });

    return (it != mNodeContainer.end());
}

/**
 * Überprüft, ob einem Knoten die Berechtigung erteilt wurde (ob er erlaubt ist).
 *
 * @param id ID des Knotens.
 * @return bool Gibt true zurück, wenn der Knoten erlaubt ist, sonst false.
 */
bool MySQLConnection::isAllowed(const std::string& id)
{
    // Suche die ID im Container
    auto it = std::find_if(mNodeContainer.begin(), mNodeContainer.end(),
        [&id](const Node& node)
        {
            return node.id == id;
        });

    if (it != mNodeContainer.end())
    {
        return it->allowed;
    }
    else
    {
        // Optional: Fehlerbehandlung, wenn die ID nicht im Container gefunden wurde
        std::cerr << "Error: Node with ID " << id << " not found in container." << std::endl;
        return false; // Default-Wert oder werfen Sie eine Ausnahme, je nach Anwendungslogik
    }
}

/**
 * Überprüft periodisch und verarbeitet die Audit-Tabelle, um Knoten zu aktivieren oder zu deaktivieren.
 */
void MySQLConnection::pollAuditTable()
{
    try
    {
        sql::PreparedStatement* selectStmt;
        std::string selectQuery = "SELECT * FROM audit";
        selectStmt = connection_->prepareStatement(selectQuery);
    
        sql::ResultSet* result = selectStmt->executeQuery();
    
        std::vector<std::string> nodeIdsToDelete;
    
        while (result->next())
        {
            std::string node_id = result->getString("node_id");
            bool allowed = result->getBoolean("allowed_value");

            // Aktiviere oder deaktiviere den Knoten
            setNodeActive(node_id, allowed, false);
    
            nodeIdsToDelete.push_back(node_id);
        }
    
        for (const auto& node_id : nodeIdsToDelete)
        {
            // Lösche den Eintrag aus der audit-Tabelle
            sql::PreparedStatement* deleteStmt;
            std::string deleteQuery = "DELETE FROM audit WHERE node_id = ?";
            deleteStmt = connection_->prepareStatement(deleteQuery);
            deleteStmt->setString(1, node_id);
            deleteStmt->executeUpdate();
            delete deleteStmt;
        }
    
        delete result;
        delete selectStmt;
    }
    catch (const sql::SQLException& e)
    {
        std::cerr << "MySQL Error: " << e.what() << std::endl;
    }
}

/**
 * Lädt alle Knoten aus der Datenbank und speichert sie im mNodeContainer.
 *
 * @return bool Gibt true zurück, wenn das Laden erfolgreich war, andernfalls false.
 */
bool MySQLConnection::fetchAllNodesFromDatabase()
{
    try
    {
        // Löscht den aktuellen Audit-Verlauf
        sql::PreparedStatement* deleteStmt;
        deleteStmt = connection_->prepareStatement("DELETE FROM audit");
        deleteStmt->executeUpdate();
        delete deleteStmt;

        // Vorbereiten der SQL-Abfrage, um alle Knotendaten zu holen
        sql::PreparedStatement* stmt;
        stmt = connection_->prepareStatement("SELECT id, online, allowed, lastSeen FROM nodes");
        sql::ResultSet* result = stmt->executeQuery();

        // Verarbeitung der Abfrageergebnisse
        while (result->next())
        {
            Node node;
            node.id = result->getString("id");
            node.online = result->getBoolean("online");
            node.allowed = result->getBoolean("allowed");
            node.lastSeen = static_cast<time_t>(result->getInt64("lastSeen")); // Assuming you store lastSeen as a timestamp in the DB

            mNodeContainer.push_back(node);

            // Setzt den Online-Status jedes Knotens auf false nach dem Laden
            setNodeOnline(node.id, false);
        }

        delete result;
        delete stmt;
        return true;
    }
    catch (const sql::SQLException& e)
    {
        std::cerr << "MySQL Error: " << e.what() << std::endl;
        return false;
    }
}

/**
 * Fügt einen neuen Knoten zum mNodeContainer hinzu, wenn er nicht bereits existiert.
 *
 * @param id ID des hinzuzufügenden Knotens.
 * @return bool Gibt true zurück, wenn der Knoten erfolgreich hinzugefügt wurde, andernfalls false.
 */
bool MySQLConnection::addNodeToContainer(std::string id)
{
    // Überprüft, ob der Knoten bereits im Container ist
    auto it = std::find_if(mNodeContainer.begin(), mNodeContainer.end(),
        [&id](const Node& node)
        {
            return node.id == id;
        });

    if (it != mNodeContainer.end()) 
    {
        // Knoten existiert bereits im Container
        return false;
    }

    // Fügt neuen Knoten zum Container hinzu
    Node newNode;
    newNode.id = id;
    newNode.lastSeen = std::time(nullptr);
    newNode.allowed = false;
    newNode.online = false;
    mNodeContainer.push_back(newNode);
    return true;
}

/**
 * Aktualisiert das "lastSeen"-Datum eines Knotens im Container und in der Datenbank.
 *
 * @param id ID des Knotens, dessen Datum aktualisiert werden soll.
 */
void MySQLConnection::setLastSeen(std::string id)
{
    // Findet den Knoten im Container
    auto it = std::find_if(mNodeContainer.begin(), mNodeContainer.end(),
        [&id](const Node& node)
        {
            return node.id == id;
        });

    // Setzt das Datum und aktualisiert die Datenbank
    if (it != mNodeContainer.end()) 
    {
        it->lastSeen = std::time(nullptr);

        // Konvertiere time_t in ein timestamp-Format für die Datenbank
        std::tm* tm_lastSeen = std::localtime(&(it->lastSeen));
        char buffer[20];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_lastSeen);
        std::string str_lastSeen(buffer);

        // Aktualisiert das "lastSeen"-Datum in der Datenbank
        try
        {
            sql::PreparedStatement* updateStmt;
            updateStmt = connection_->prepareStatement("UPDATE nodes SET lastSeen = ? WHERE id = ?");
            updateStmt->setString(1, str_lastSeen);
            updateStmt->setString(2, id);
            updateStmt->executeUpdate();
            delete updateStmt;
        }
        catch (const sql::SQLException& e)
        {
            std::cerr << "MySQL Error while updating lastSeen for id " << id << ": " << e.what() << std::endl;
        }
    }
}

/**
 * Entfernt einen Knoten aus dem mNodeContainer.
 *
 * @param id ID des zu entfernenden Knotens.
 */
void MySQLConnection::removeNodeFromContainer(std::string id)
{
    // Sucht den Knoten im Container
    auto it = std::find_if(mNodeContainer.begin(), mNodeContainer.end(),
        [&id](const Node& node)
        {
            return node.id == id;
        });

    if (it != mNodeContainer.end()) 
    {
        mNodeContainer.erase(it);
    }
}

/**
 * Überwacht die "lastSeen"-Daten aller Knoten und aktualisiert ihren Online-Status.
 * Knoten, die länger als 60 Sekunden nicht gesehen wurden, werden als offline markiert.
 */
void MySQLConnection::monitorLastSeen()
{
    time_t currentTime = std::time(nullptr);
    for (const auto& node : mNodeContainer) 
    {
        if (node.online && (currentTime - node.lastSeen > 60)) 
            setNodeOnline(node.id, false);
    }

    // Überprüft die Audit-Tabelle auf Änderungen
    pollAuditTable();
}
