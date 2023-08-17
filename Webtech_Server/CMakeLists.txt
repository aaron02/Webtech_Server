# Mindestversion von CMake festlegen
cmake_minimum_required(VERSION 3.10)

# Projektname
project(Webtech_Server)

# Setze die C++-Standardversion
set(CMAKE_CXX_STANDARD 20)

# Füge den Pfad zu den Paho MQTT-Headerdateien hinzu
include_directories("extern/paho-mqtt/include")

# Füge die Paho MQTT-Bibliotheken für Linux hinzu
find_library(PAHO_MQTT_CPP_LIB libpaho-mqttpp3.a REQUIRED PATHS "${CMAKE_SOURCE_DIR}/paho-mqtt/lib")
find_library(PAHO_MQTT_C_LIB libpaho-mqtt3a.a REQUIRED PATHS "${CMAKE_SOURCE_DIR}/paho-mqtt/lib")

# Füge pthread-Bibliothek hinzu
find_package(Threads REQUIRED)

# Finde und verlinke den MySQL Connector-C++
#find_package(mysqlcppconn REQUIRED)

# Füge die ausführbare Datei diesem Projekt hinzu
# Quelldateien aus dem aktuellen Ordner sammeln
file(GLOB CURRENT_SOURCES *.cpp *.h)

# Quelldateien aus dem Unterordner "MQTT Base" rekursiv sammeln
file(GLOB_RECURSE MQTT2_SOURCES MQTT/BaseClasses/*.cpp MQTT/BaseClasses/*.h)

# Quelldateien aus dem Unterordner "MQTT" rekursiv sammeln
file(GLOB_RECURSE MQTT_SOURCES MQTT/*.cpp MQTT/*.h)

# Quelldateien aus dem Unterordner "MySQL" rekursiv sammeln
file(GLOB_RECURSE MYSQL_SOURCES MySQL/*.cpp MySQL/*.h)

# Füge die ausführbare Datei mit all diesen Quelldateien hinzu
add_executable(Webtech_Server ${CURRENT_SOURCES} ${MQTT2_SOURCES} ${MQTT_SOURCES} ${MYSQL_SOURCES})

# Füge die Header-Verzeichnisse für MySQL hinzu
# include_directories(${MYSQLCPPCONN_INCLUDE_DIRS})
#include_directories("extern/mysql")
#include_directories("extern/mysqlx")
#include_directories("extern/jdbc")

# Füge den Pfad zur nlohmann JSON-Bibliothek hinzu
include_directories("extern/nlohmann")

# Verlinke die erforderlichen Bibliotheken
#target_link_libraries(Webtech_Server PRIVATE ${PAHO_MQTT_CPP_LIB} ${PAHO_MQTT_C_LIB} Threads::Threads ${MYSQLCPPCONN_LIBRARIES})
#target_link_libraries(Webtech_Server PRIVATE ${PAHO_MQTT_CPP_LIB} ${PAHO_MQTT_C_LIB} Threads::Threads mysqlcppconn)
target_link_libraries(Webtech_Server PRIVATE -lmysqlcppconn ${PAHO_MQTT_CPP_LIB} ${PAHO_MQTT_C_LIB} Threads::Threads)

