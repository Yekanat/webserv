// ********** MAIN **********
// Entry point: carica config e avvia parser

#include <iostream>
#include <string>
#include "ConfigParser.hpp"
#include "ServerInstance.hpp"
#include "Server.hpp"
#include <vector>
#include <map>

int main(int argc, char **argv)
{
    std::string config_path = "conf/default.conf";
    if (argc == 2)
        config_path = argv[1];

    try {
        std::cout << "Avvio webserv con config: " << config_path << std::endl;
        ConfigParser parser(config_path);
        parser.parse();

        const std::vector<ServerConfig>& servers = parser.getServers();
        std::vector<ServerInstance*> instances;
        Server webserver;
        
        // Aggiungi i server alla configurazione
        webserver.setServers(servers);

        // Traccia socket già creati per evitare duplicati
        std::map<std::pair<std::string, int>, ServerInstance*> uniqueSockets;

        // Crea i socket e aggiungi le istanze
        for (size_t i = 0; i < servers.size(); ++i) {
            const ServerConfig& srv = servers[i];
            
            for (size_t j = 0; j < srv.listen.size(); ++j) {
                const std::string& host = srv.listen[j].first;
                int port = srv.listen[j].second;
                std::pair<std::string, int> endpoint(host, port);
                
                // Controlla se questo socket esiste già
                ServerInstance* instance;
                if (uniqueSockets.find(endpoint) == uniqueSockets.end()) {
                    try {
                        instance = new ServerInstance(host, port);
                        instances.push_back(instance);
                        uniqueSockets[endpoint] = instance;
                        // Rimosso print duplicato - il constructor già stampa
                    } catch (const std::exception& e) {
                        std::cerr << "Errore binding " << host << ":" << port 
                                  << " - " << e.what() << std::endl;
                        continue;  // Salta questa configurazione
                    }
                } else {
                    instance = uniqueSockets[endpoint];
                }
                
                // Aggiungi la configurazione del server all'istanza
                webserver.addInstance(instance, srv);
            }
        }

        // Avvia il server
        webserver.run();

        // Cleanup
        for (size_t i = 0; i < instances.size(); ++i)
            delete instances[i];

    } catch (const ConfigException& e) {
        std::cerr << "Errore di configurazione: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Errore: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
