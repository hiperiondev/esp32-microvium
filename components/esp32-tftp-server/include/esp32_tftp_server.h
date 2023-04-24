/*
 * @file esp32_tftp_server.h
 * @brief TFTP server API
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef TFTP_SERVER_H_
#define TFTP_SERVER_H_

#define TFTP_DEFAULT_PORT (69)

#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>

void TFTP_init(uint16_t port);

/**
 * Starts server.
 * Returns 0 if server is started successfully
 */
int TFTP_start(void);

/**
 * Listens for incoming requests. Can be executed in blocking and non-blocking mode
 * @param wait_for true if blocking mode is required, false if non-blocking mode is required
 */
int TFTP_run(bool wait_for);

/**
 * Stops server
 */
void TFTP_stop(void);

void TFTP_task_start(void);
void TFTP_task_stop(void);

#endif /* TFTP_SERVER_H_ */
