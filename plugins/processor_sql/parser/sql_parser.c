/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fluent-bit/flb_info.h>
#include <fluent-bit/flb_log.h>
#include <fluent-bit/flb_mem.h>
#include <fluent-bit/flb_str.h>

#include "processor-sql_parser.h"
#include "processor-sql-parser_lex.h"

#include "sql.h"

//#include "sql_parser.h"
//#include "sql_lex.h"

// void flb_sp_cmd_destroy(struct flb_sp_cmd *cmd)
// {
//     struct mk_list *head;
//     struct mk_list *tmp;
//     struct flb_sp_cmd_key *key;

//     /* remove keys */
//     mk_list_foreach_safe(head, tmp, &cmd->keys) {
//         key = mk_list_entry(head, struct flb_sp_cmd_key, _head);
//         mk_list_del(&key->_head);
//         flb_sp_cmd_key_del(key);
//     }

//     flb_free(cmd->source_name);
//     flb_free(cmd);
// }

// void flb_sp_cmd_key_del(struct flb_sp_cmd_key *key)
// {
//     if (key->name) {
//         flb_sds_destroy(key->name);
//     }
//     if (key->alias) {
//         flb_sds_destroy(key->alias);
//     }
//     flb_free(key);
// }

// int flb_sp_cmd_key_add(struct flb_sp_cmd *cmd, int aggr_func,
//                        char *key_name, char *key_alias)
// {
//     struct flb_sp_cmd_key *key;

//     key = flb_calloc(1, sizeof(struct flb_sp_cmd_key));
//     if (!key) {
//         flb_errno();
//         return -1;
//     }

//     /* key name and aliases works when the selection is not a wildcard */
//     if (key_name) {
//         key->name = flb_sds_create(key_name);
//         if (!key->name) {
//             flb_sp_cmd_key_del(key);
//             return -1;
//         }
//     }
//     else {
//         /*
//          * This is a wildcard selection, make sure if any aggregation function
//          * exists only apply for COUNT().
//          */
//         if (aggr_func > 0  && aggr_func != FLB_SP_COUNT) {
//             flb_sp_cmd_key_del(key);
//             return -1;
//         }
//     }

//     if (key_alias) {
//         key->alias = flb_sds_create(key_alias);
//         if (!key->alias) {
//             flb_sp_cmd_key_del(key);
//             return -1;
//         }
//     }

//     /* Aggregation function */
//     if (aggr_func > 0) {
//         key->aggr_func = aggr_func;
//     }

//     mk_list_add(&key->_head, &cmd->keys);
//     return 0;
// }

// int flb_sp_cmd_source(struct flb_sp_cmd *cmd, int type, char *source)
// {
//     cmd->source_type = type;
//     cmd->source_name = flb_strdup(source);
//     if (!cmd->source_name) {
//         flb_errno();
//         return -1;
//     }

//     return 0;
// }

// void flb_sp_cmd_dump(struct flb_sp_cmd *cmd)
// {
//     struct mk_list *head;
//     struct mk_list *tmp;
//     struct flb_sp_cmd_key *key;

//     /* Lookup keys */
//     printf("== KEYS ==\n");
//     mk_list_foreach_safe(head, tmp, &cmd->keys) {
//         key = mk_list_entry(head, struct flb_sp_cmd_key, _head);
//         printf("- '%s'\n", key->name);
//     }
//     printf("== SOURCE ==\n");
//     if (cmd->source_type == FLB_SP_STREAM) {
//         printf("stream => ");
//     }
//     else if (cmd->source_type == FLB_SP_TAG) {
//         printf("tag match => ");
//     }

//     printf("'%s'\n", cmd->source_name);
// }

int sql_parser_query_key_add(struct sql_query *query, char *key_name, char *key_alias)
{
    struct sql_key *key;

    key = flb_calloc(1, sizeof(struct sql_key));
    if (!key) {
        flb_errno();
        return -1;
    }

    key->name = flb_sds_create(key_name);
    if (!key->name) {
        flb_free(key);
        return -1;
    }

    if (key_alias) {
        key->alias = flb_sds_create(key_alias);
        if (!key->alias) {
            flb_sds_destroy(key->name);
            flb_free(key);
            return -1;
        }
    }

    cfl_list_add(&key->_head, &query->keys);
    return 0;
}

void sql_parser_query_destroy(struct sql_query *query)
{
    struct cfl_list *head;
    struct cfl_list *tmp;
    struct sql_key *key;

    cfl_list_foreach_safe(head, tmp, &query->keys) {
        key = cfl_list_entry(head, struct sql_key, _head);
        cfl_list_del(&key->_head);
        flb_free(key);
    }

    flb_free(query);
}

struct sql_query *sql_parser_query_create(cfl_sds_t query_str)
{
    int ret;
    yyscan_t scanner;
    YY_BUFFER_STATE buf;
    struct sql_query *query;

    query = flb_calloc(1, sizeof(struct sql_query));
    if (!query) {
        flb_errno();
        return NULL;
    }
    cfl_list_init(&query->keys);
    cfl_list_init(&query->cond_list);

     /* Flex/Bison work */
    yylex_init(&scanner);
    buf = yy_scan_string(query_str, scanner);

    ret = yyparse(query, scanner);
    if (ret != 0) {
        sql_parser_query_destroy(query);
        return NULL;
    }

    yy_delete_buffer(buf, scanner);
    yylex_destroy(scanner);

    return query;
}
