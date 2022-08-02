/*
 * ======================================================================
 *       Filename:  json_config_parser.c
 *         Author:  doctorgero (ag), m4pun4@protonmail.com
 *   Organization:  The SAAS Group (UK)
 * ======================================================================
 */
#include "config_parser.h"

struct config *config_parser(const char *config_file_path) {
    size_t rd;
    yajl_val node;
    unsigned char filedata[65536];
    char errbuf[1024];
    struct sockaddr_in sa;
    struct config *cfg = (struct config *) malloc(sizeof(struct config));
    struct ipconfigs *cameras;

    FILE *fp = fopen(config_file_path, "r");

    filedata[0] = errbuf[0] = 0;
    rd = fread((void *) filedata, 1, sizeof(filedata) - 1, fp);

    if (rd == 0 && !feof(fp)) {
        fprintf(stderr, "error reading file\n");
        return NULL;
    } else if (rd >= sizeof(filedata) - 1) {
        fprintf(stderr, "Config file is too big\n");
        return NULL;
    }

    node = yajl_tree_parse((const char *) filedata, errbuf, sizeof(errbuf));

    /* error handling */
    if (node == NULL) {
        fprintf(stderr, "parse error: ");
        if (strlen(errbuf))
            fprintf(stderr, " %s", errbuf);
        else
            fprintf(stderr, "unknown error");
        return NULL;
    }

    /* YAJL Tree Block */
    {
        const char *p1[] = {"image_dir", (const char *) 0};
        yajl_val dir = yajl_tree_get(node, p1, yajl_t_string);

        if (dir) {
            cfg->img_dir = (char *) malloc(strlen(YAJL_GET_STRING(dir)));
            strcpy(cfg->img_dir, YAJL_GET_STRING(dir));
        }

        const char *p2[] = {"cameras", (const char *) 0};
        yajl_val v = yajl_tree_get(node, p2, yajl_t_object);

        if (v && YAJL_IS_OBJECT(v)) {
            cfg->n_config = v->u.object.len;
            cameras = (struct ipconfigs *) malloc(cfg->n_config * sizeof(struct ipconfigs));
            printf("# cameras configured (in %s): %ld\n", config_file_path, cfg->n_config);

            int enabled = 0;

            for (size_t i = 0; i < cfg->n_config; i++) {
                const char *key = v->u.object.keys[i];
                printf("%s: ", key);
                yajl_val vv = v->u.object.values[i];

                size_t sublen = vv->u.object.len;

                for (size_t ii = 0; ii < sublen; ii++) {
                    key = vv->u.object.keys[ii];
                    yajl_val vvv = vv->u.object.values[ii];
                    if (strcmp(key, "enabled") == 0 && YAJL_IS_TRUE(vvv)) {
                        cameras[i].valid = 1;
                        enabled++;
                        printf("enabled, ");
                    }

                    if (strcmp(key, "ip") == 0 && YAJL_IS_STRING(vvv)) {
                        const char *ipaddr = YAJL_GET_STRING(vvv);
                        if (inet_pton(AF_INET, ipaddr, &(sa.sin_addr)) == 0) {
                            fprintf(stderr, "Wrong IPv4 address provided: %s\n", ipaddr);
                            return NULL;
                        } else {
                            strcpy(cameras[i].ipaddr, ipaddr);
                            printf("IP: %s, ", ipaddr);
                        }
                    }

                    if (strcmp(key, "port") == 0 && YAJL_IS_INTEGER(vvv)) {
                        int port = YAJL_GET_INTEGER(vvv);
                        if (port < 0 || port > 65535) {
                            fprintf(stderr, "Illegal port number: %d\n", port);
                            exit(EXIT_FAILURE);
                        } else {
                            sprintf(cameras[i].port, "%d", port);
                            printf("Port: %s, ", cameras[i].port);
                        }
                    } else { // Default port 20 if no port configured.
                        sprintf(cameras[i].port, "%d", 20);
                    }

                    if (strcmp(key, "stream") == 0 && YAJL_IS_INTEGER(vvv)) {
                        int stream = YAJL_GET_INTEGER(vvv);
                        if (stream == 0 || stream == 1) {
                            cameras[i].strm = stream;
                            printf("Stream: %d\n", cameras[i].strm);
                        } else {
                            fprintf(stderr, "Illegal stream: %d\n", stream);
                            exit(EXIT_FAILURE);
                        }
                    } else { // Default stream, if no stream is configured
                        cameras[i].strm = 0;
                    }
                }
            }

            printf("\n\n# cameras enabled: %d\n", enabled);
        } else {
            printf("no config found for: %s\n", p2[0]);
            return NULL;
        }
    }

    yajl_tree_free(node);
    fclose(fp);
    cfg->configs = cameras;

    return cfg;
}

/*
   int main(int argc, char *argv[]) {
   struct config *c = config_parser(argv[1]);
   struct ipconfigs *cameras = c->configs;
   int n_configs;

   if (cameras != NULL) {
   printf("Directory: %s\n", c->img_dir);
   free(cameras);
   } else {
   printf("NULL\n");
   }

   return 0;
   }
   */
