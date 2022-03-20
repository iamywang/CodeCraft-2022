#!/usr/bin/python3
import math
import numpy as np

cname, sname, qos, qos_lim = None, None, None, None
client_demand = None
bandwidth = None
time_total = None
cname_map = {}
sname_map = {}

input_data_path = '/data/'

class IOFile():
    demand = input_data_path + 'demand.csv'
    qos = input_data_path + 'qos.csv'
    bandwidth = input_data_path + 'site_bandwidth.csv'
    config = input_data_path + 'config.ini'
    output = '/output/solution.txt'

def read_demand():
    fname = IOFile.demand
    with open(fname) as f:
        data = f.read().splitlines()
    client_name = data[0].split(',')[1:]
    client_demand = []
    time_total = []
    for each in data[1:]:
        d = each.split(',')
        time_total.append(d[0])
        client_demand.append(list(map(int, d[1:])))
    return time_total, client_name, client_demand

def read_server_bandwidth():
    fname = IOFile.bandwidth
    with open(fname) as f:
        data = f.read().splitlines()
    server_name = []
    server_bandwidth = []
    for each in data[1:]:
        a, b = each.split(',')
        server_name.append(a)
        server_bandwidth.append(int(b))
    return server_name, server_bandwidth

def read_qos():
    fname = IOFile.qos
    with open(fname) as f:
        data = f.read().splitlines()
    client_name = data[0].split(',')[1:]
    server_name = []
    qos_array = []
    for each in data[1:]:
        d = each.split(',')
        server_name.append(d[0])
        qos_array.append(list(map(int, d[1:])))
    return client_name, server_name, qos_array

def read_qos_limit():
    fname = IOFile.config
    with open(fname) as f:
        data = f.read().splitlines()
    qos_lim = int(data[1].split('=')[1])
    return qos_lim

def get_input_data():
    global cname, sname, qos, qos_lim, bandwidth, client_demand, time_total
    cname, sname, qos = read_qos()
    for idx, name in enumerate(cname):
        cname_map[name] = idx
    for idx, name in enumerate(sname):
        sname_map[name] = idx
    qos = np.array(qos)
    time_total, client_name, client_demand = read_demand()
    client_idx_list = []
    for c in cname:
        idx = client_name.index(c)
        client_idx_list.append(idx)
    client_demand = np.array(client_demand)[:, client_idx_list]
    server_name, server_bandwidth = read_server_bandwidth()
    bandwidth = []
    for s in sname:
        idx = server_name.index(s)
        bandwidth.append(server_bandwidth[idx])
    qos_lim = read_qos_limit()
    bandwidth = np.array(bandwidth)

class SolutionCheck():
    def __init__(self):
        self.server_history_bandwidth = []
        self.max = len(cname)
        self.curr_time = -1
        self.record = np.zeros((len(time_total), len(sname), len(cname)), dtype=np.int32)
        self.client_outputed = [ False for _ in range(len(cname)) ]
        self.server_used_bandwidth = np.zeros(len(sname), dtype=np.int64)
        self.count = 0
        self.curr_time += 1

    def read_and_check(self, line: str):
        # client node
        c, remain = line.strip().split(':')
        c_idx = cname_map.get(c)
        self.client_outputed[c_idx] = True
        self.count += 1

        # server node
        if remain.strip() == '':
            if client_demand[self.curr_time, c_idx] != 0:
                print(f'Not Satisfied: Time: {self.curr_time}, Client: {cname[c_idx]}, Demand: {client_demand[self.curr_time, c_idx]}, Allocated: 0')
            self.time_check()
            return
        dispatchs = remain[1: -1].split(',')
        if len(dispatchs) == 2:
            s, res = dispatchs
            self.server_node_check(c_idx, s, res)
            if int(res) != client_demand[self.curr_time, c_idx]:
                print(f'Not Satisfied: Time: {self.curr_time}, Client: {cname[c_idx]}, Demand: {client_demand[self.curr_time, c_idx]}, Allocated: {res}')
            self.time_check()
            return
        dispatchs = remain[1: -1].split('>,<')
        res_accum = 0
        for d_str in dispatchs:
            s, res = d_str.split(',')
            self.server_node_check(c_idx, s, res)
            res_accum += int(res)
        if res_accum != client_demand[self.curr_time, c_idx]:
                print(f'Not Satisfied: Time: {self.curr_time}, Client: {cname[c_idx]}, Demand: {client_demand[self.curr_time, c_idx]}, Allocated: {res}')
        self.time_check()
    
    def server_node_check(self, c_idx: int, server_name: str, res_str: str):
        s_idx = sname_map.get(server_name)
        res = int(res_str)
        self.record[self.curr_time, s_idx, c_idx] += res
        self.server_used_bandwidth[s_idx] += res
        if self.server_used_bandwidth[s_idx] > bandwidth[s_idx]:
            print(f'Not Satisfied: Time: {self.curr_time}, Server: {server_name}, Client: {cname[c_idx]}, Used: {self.server_used_bandwidth[s_idx]}, Limit: {bandwidth[s_idx]}')
        if qos[s_idx, c_idx] >= qos_lim:
            print(f'Not Satisfied: Time: {self.curr_time}, Server: {server_name}, Client: {cname[c_idx]},  QoS: {qos[s_idx, c_idx]}, Limit: {qos_lim}')
    
    def time_check(self):
        if self.count == self.max:
            self.server_history_bandwidth.append(self.server_used_bandwidth)
            self.client_outputed = [ False for _ in range(len(cname)) ]
            self.server_used_bandwidth = np.zeros(len(sname), dtype=np.int64)
            self.count = 0
            self.curr_time += 1
    
    def read_file(self, output_file_name: str):
        with open(output_file_name) as f:
            lines = f.read().splitlines()
        for l_idx, l in enumerate(lines):
            self._curr_read_line = l
            self._curr_line_idx = l_idx
            self.read_and_check(l)
        if self.curr_time != len(time_total):
            print(f'Not Satisfied: Times solved: {self.curr_time}, Times total: {len(time_total)}')
    
    def print_result(self):
        time_cnt = len(time_total)
        idx = math.ceil(time_cnt * 0.95) - 1
        server_history = np.array(self.server_history_bandwidth)
        server_history.sort(axis=0)
        score = server_history[idx].sum()
        print(f'Total Costs: {score}')

if __name__ == '__main__':
    get_input_data()
    checker = SolutionCheck()
    checker.read_file(IOFile.output)
    checker.print_result()
