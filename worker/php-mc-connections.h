#pragma once

#include "net/net-memcache-client.h"

#include "PHP/worker/php-queries.h"
#include "PHP/worker/php-worker.h"

void php_worker_run_mc_query_packet(php_worker *worker, php_net_query_packet_t *query);
extern conn_target_t memcache_ct;