/*
 * Copyright 2023 Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * * Project Site: https://github.com/hiperiondev/esp32-microvium *
 *
 * This is based on other projects:
 *    Microvium (https://github.com/coder-mike/microvium)
 *    Others (see individual files)
 *
 *    please contact their authors for more information.
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
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
