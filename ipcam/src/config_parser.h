/*
 * ======================================================================
 *       Filename:  config_parser.h
 *    Description:  header for config parser
 *        Version:  1.0
 *        Created:  23/07/19 05:21:28 PM IST
 *       Revision:  none
 *         Author:  doctorgero (ag), m4pun4@protonmail.com
 *   Organization:  
 * ======================================================================
 */
#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>

#include <yajl/yajl_tree.h>

#include "threadedClientSocket.h"

struct config {
    char *img_dir;
    size_t n_config;
    struct ipconfigs *configs;
};

struct config *config_parser(const char *config_file_apth);

# endif
