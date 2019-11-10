/*
 ============================================================================
 Name        : ads1115_example.c
 Author      : Giovanni Bauermeister
 Description : Read analog values from potentiometer using ADS1115 and prints to terminal
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>

#include "mqtt.h"
#include "ads1115_rpi.h"
#include "posix_sockets.h"

void* client_refresher(void* client);
void exit_example(int status, int sockfd, pthread_t *client_daemon);
void publish_callback(void** unused, struct mqtt_response_publish *published);

int main(void) {
	if(openI2CBus("/dev/i2c-1") == -1) {
		return EXIT_FAILURE;
	}
	setI2CSlave(0x48);

    int sockfd = open_nb_socket("localhost", "1883");

    if (sockfd == -1) {
        perror("Failed to open socket: ");
        exit_example(EXIT_FAILURE, sockfd, NULL);
    }

    /* setup a client */
    struct mqtt_client client;
    uint8_t sendbuf[4096]; /* sendbuf should be large enough to hold multiple whole mqtt messages */
    uint8_t recvbuf[1024]; /* recvbuf should be large enough any whole mqtt message expected to be received */
    mqtt_init(&client, sockfd, sendbuf, sizeof(sendbuf), recvbuf, sizeof(recvbuf), publish_callback);
    mqtt_connect(&client, "publishing_client", NULL, NULL, 0, NULL, NULL, 0, 400);

    /* check that we don't have any errors */
    if (client.error != MQTT_OK) {
        fprintf(stderr, "error: %s\n", mqtt_error_str(client.error));
        exit_example(EXIT_FAILURE, sockfd, NULL);
    }

    /* start a thread to refresh the client (handle egress and ingree client traffic) */
    pthread_t client_daemon;
    if(pthread_create(&client_daemon, NULL, client_refresher, &client)) {
        fprintf(stderr, "Failed to start client daemon.\n");
        exit_example(EXIT_FAILURE, sockfd, NULL);

    }

    /* start publishing the time */
    printf("is ready to begin publishing the time.\n");
    printf("Press ENTER to publish the current time.\n");
    while(1) {
        /* print a message */
        char application_message[256];
        snprintf(application_message, sizeof(application_message), "%.2f", readVoltage(0));
        printf("published: \"%s\"\n", application_message);

        /* publish the time */
        mqtt_publish(&client, "valv-water-level", application_message, strlen(application_message), MQTT_PUBLISH_QOS_0);

        /* check for errors */
        if (client.error != MQTT_OK) {
            fprintf(stderr, "error: %s\n", mqtt_error_str(client.error));
            exit_example(EXIT_FAILURE, sockfd, &client_daemon);
        }
        
        usleep(1000000);
    }   

    /* disconnect */
    printf("\n disconnecting from \n");
    sleep(1);

    /* exit */ 
    exit_example(EXIT_SUCCESS, sockfd, &client_daemon);
}

void exit_example(int status, int sockfd, pthread_t *client_daemon) {
    if (sockfd != -1) close(sockfd);
    if (client_daemon != NULL) pthread_cancel(*client_daemon);
    exit(status);
}

void publish_callback(void** unused, struct mqtt_response_publish *published) {
    /* not used in this example */
}


void* client_refresher(void* client) {
    while(1) {
        mqtt_sync((struct mqtt_client*) client);
        usleep(100U);
    }
    return NULL;
}
