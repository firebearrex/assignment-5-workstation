/*
 * http_server.c
 *
 * The HTTP server main function sets up the listener socket
 * and dispatches client requests to request sockets.
 *
 *  @since 2019-04-10
 *  @author: Philip Gust
 */
#include <stdbool.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "http_methods.h"
#include "time_util.h"
#include "http_util.h"
#include "http_request.h"
#include "network_util.h"
#include "properties.h"
#include "http_server.h"
#include "thpool.h"
#include "media_util.h"
#include "file_util.h"

/**
 * The port numbers come from wikipedia and they are registered ports.
 * Soure: https://en.wikipedia.org/wiki/Port_(computer_networking)
 */
#define MIN_PORT 1024
#define MAX_PORT 49151

#define DEFAULT_HTTP_PORT 8080

/** http server configuration */
struct http_server_conf server;


/**
 * Process the server configuration file
 * @param configFileName name of the configuration file
 * @return true if successful, false if error occurred
 */
static bool process_config(const char* configFileName) {
	bool status = true;
	Properties* httpConfig = newProperties();

	do {
		// load properties from config file
		if (loadProperties(configFileName, httpConfig) == 0) {
			fprintf(stderr, "Missing configuration file '%s'\n", configFileName);
			status = false;
			break;
		}

		// initialize debug flag
		char debugProp[MAXBUF];
		if (findProperty(httpConfig, 0, "Debug", debugProp) != SIZE_MAX) {
			server.debug =  (strcasecmp(debugProp, "true") == 0);
		}

        // server root directory relative to config file path
        char configFilePath[MAXBUF];
        if (getPath(configFileName, configFilePath) != NULL) {
            if (chdir(configFilePath) != 0) { // should not happen
                perror("configFilePath");
                status = false;
                break;
            }
        }

		// set server root directory
		char rootDirProp[MAXBUF];
		if (findProperty(httpConfig, 0, "ServerRoot", rootDirProp) != SIZE_MAX) {
			if (chdir(rootDirProp) != 0) {
				perror("setServerRoot");
				status = false;
				break;
			}
		}

		// initialize the listener port
		server.server_port = DEFAULT_HTTP_PORT;
		char listenProp[MAXBUF];
		if (findProperty(httpConfig, 0, "Port", listenProp) != SIZE_MAX) {
			if (   (sscanf(listenProp, "%d", &server.server_port) != 1)
				|| (server.server_port < MIN_PORT)
				|| (server.server_port > MAX_PORT)) {
				fprintf(stderr, "Invalid port %s\n", listenProp);
				status = false;
				break;
			}
		}

		// set content base property if specified or use default "content"
		static char contentBaseProp[MAXBUF] = "content";
		server.content_base = contentBaseProp;
		findProperty(httpConfig, 0, "ContentBase", contentBaseProp);

		// set server host property or use default "localhost"
		static char serverHostProp[MAXBUF] = "localhost";
		server.server_host = serverHostProp;
		findProperty(httpConfig, 0, "ServerHost", serverHostProp);

		// set default server name property if not set
		static char serverNameProp[MAXBUF];
		server.server_name = serverNameProp;
		if (findProperty(httpConfig, 0, "ServerName", serverNameProp) == SIZE_MAX) {
			// set default server name as serverhost:port
			sprintf(serverNameProp,"%s:%d", server.server_host, server.server_port);
		}

		// set server response protocol property or use default "HTTP/1.1"
		static char serverProtocolProp[MAXBUF] = "HTTP/1.1";
		server.server_protocol = serverProtocolProp;
		findProperty(httpConfig, 0, "ServerProtocol", serverProtocolProp);

	} while(false);

	deleteProperties(httpConfig);
	return status;
}

/**
 * Main program starts the server and processes requests
 * @param argc argument count
 * @param argv array of args; argv[1] may be port no.
 */
int main(int argc, char* argv[argc]) {

	// initialize config file name
	const char* configFileName = "httpd.conf";
    if (argc == 2) {
    	configFileName = argv[1];
    }

	// load property file with server configuration
	if (!process_config(configFileName)) {
		return EXIT_FAILURE;
	}

    // create listener socket for server with specified port
    int listen_sock_fd = get_listener_socket(server.server_port);
	if (listen_sock_fd == 0) {
		perror("listen_sock_fd");
		return EXIT_FAILURE;
	}

	if (server.debug) {
		fprintf(stderr, "HttpServer running on port %d\n", server.server_port);
	}

    threadpool thpool = thpool_init(4);

	while (true) {
        // accept client connection
		int socket_fd = accept_peer_connection(listen_sock_fd);

		if (server.debug) {
			int port;
			char host[MAXBUF];
			if (get_peer_host_and_port(socket_fd, host, &port) != 0) {
			    perror("get_peer_host_and_port");
			} else if (server.debug) {
				fprintf(stderr, "New connection accepted  %s:%u\n", host, port);
			}
		}

		// handle request
//		process_request(socket_fd);
        thpool_add_work(thpool, param_adapter, (void*)(uintptr_t)socket_fd);
    }

    thpool_destroy(thpool);

    // close listener socket
    close(listen_sock_fd);
    return EXIT_SUCCESS;

}
