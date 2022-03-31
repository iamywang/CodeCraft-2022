#pragma once

#include "../utils/utils.hpp"
#include "../global.hpp"

void read_configure(int &qos_constraint, int &minimum_cost);

void read_demand(DEMAND &demand);

void read_qos(QOS &qos);

void read_site_bandwidth(SITE_BANDWIDTH &site_bandwidth);
