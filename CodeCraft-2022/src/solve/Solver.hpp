#pragma once

#include <iostream>
#include <functional>
#include <numeric>
#include <algorithm>
#include "../optimize/optimize_interface.hpp"
#include "../optimize/Optimizer.hpp"
#include "../utils/tools.hpp"
#include "../utils/Verifier.hpp"
#include "../utils/Graph/MaxFlow.hpp"

#include "solve_datatype.hpp"
#include "ResultGenerator.hpp"
#include "solve_utils.hpp"
#include "XSolver.hpp"

using namespace std;

namespace solve
{

    class Solver
    {
    private:
        CommonDataForResultGenrator m_common_data;
        ResultGenerator *m_result_generator;

    public:
        Solver(std::vector<ANSWER> &X_results,
               const DEMAND &demand) : m_common_data(demand, X_results)
        {
            m_result_generator = (ResultGenerator *)new XSolverGreedyAlgorithm(m_common_data);
            // m_result_generator = (ResultGenerator *)new XSolverMaxFlow(m_common_data);
        }
        ~Solver() { delete m_result_generator; }

        int solve(const int num_iteration, const bool is_generate_initial_results)
        {
            auto &server_supported_flow_2_time_vec =
                m_common_data.m_server_supported_flow_2_time_vec;
            auto &X_results = m_common_data.m_X_results;
            auto &demand = m_common_data.m_demand;

            solve::Utils::sort_by_demand_and_qos(demand, server_supported_flow_2_time_vec);

            if (is_generate_initial_results)
            {
                m_result_generator->generate_initial_X_results();
            }
/*
            for (auto &X : X_results)
            {
                X.idx_local_mtime = demand.get(X.mtime);
            }

            //根据时间对X_results进行排序
            std::sort(X_results.begin(), X_results.end(), [this](const ANSWER &a, const ANSWER &b)
                      { return a.idx_global_mtime < b.idx_global_mtime; });

            //根据时间对server_supported_flow_2_time_vec排序
            std::sort(server_supported_flow_2_time_vec.begin(), server_supported_flow_2_time_vec.end(), [this](const vector<SERVER_SUPPORTED_FLOW> &a, const vector<SERVER_SUPPORTED_FLOW> &b)
                      { return a[0].idx_global_mtime < b[0].idx_global_mtime; });

            for (auto &v : server_supported_flow_2_time_vec)
            {
                //按照server_id排序
                std::sort(v.begin(), v.end(), [](const SERVER_SUPPORTED_FLOW &a, const SERVER_SUPPORTED_FLOW &b)
                          { return a.server_index < b.server_index; });
            }

            optimize::Optimizer(demand, X_results).optimize(server_supported_flow_2_time_vec, num_iteration);
#ifdef TEST
            if (Verifier(demand).verify(X_results))
            {
                cout << "verify success" << endl;
            }

            else
            {
                printf("verify failed\n");
                return -1;
            }
#endif
//*/
            return 0;
        }
    };

} // namespace optimize