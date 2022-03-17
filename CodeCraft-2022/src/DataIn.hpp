#pragma once

#include "tools.hpp"
#include "global.hpp"

int read_qos_constraint();

void read_demand(DEMAND &demand);

void read_qos(QOS &qos);

void read_site_bandwidth(SITE_BANDWIDTH &site_bandwidth);
