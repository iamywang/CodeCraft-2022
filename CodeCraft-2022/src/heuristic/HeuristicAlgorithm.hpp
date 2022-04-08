#pragma once

#include "../global.hpp"
#include "../generate/generate.hpp"
#include "../DivideConquer.hpp"
#include "../utils/Thread/ThreadSafeContainer.hpp"
#include "../utils/Thread/ThreadPoll.hpp"
#include "../solve/Solver.hpp"

namespace heuristic
{
    class HeuristicAlgorithm
    {
    private:
        typedef std::shared_ptr<std::vector<ANSWER>> SP_VEC_ANSWER;

        SP_VEC_ANSWER m_X_results;
        // std::vector<ANSWER>& m_X_results;

        MyUtils::Thread::ThreadSafeQueue<SP_VEC_ANSWER> m_X_results_after_dispath_queue; //优化之后的解
        // MyUtils::Thread::ThreadSafeStack<SP_VEC_ANSWER> m_X_results_after_dispath_queue;  //优化之后的解

        MyUtils::Thread::ThreadSafeStack<SP_VEC_ANSWER> m_X_results_before_dispath_stack; //优化之后的解
        std::atomic_bool m_is_running;
        MyUtils::Thread::ThreadPool m_thread_pool;

    public:
        SP_VEC_ANSWER m_best_X_results;

    public:
        HeuristicAlgorithm(std::vector<ANSWER> &X_results) : m_X_results(std::make_shared<std::vector<ANSWER>>()),
                                                             m_is_running(true),
                                                             m_thread_pool(4)
        {
            *m_X_results = std::move(X_results);
            m_thread_pool.commit([this]()
                                 { this->combine(); });
        }
        ~HeuristicAlgorithm() {}

        void optimize()
        {

            for (int i = 0; i < 10000 && m_is_running.load(); i++)
            {
                // DivideConquer::task(0, global::g_demand.client_demand.size() - 1, 300, false, *m_X_results);
                // DivideConquer::divide_conquer<false>(0, global::g_demand.client_demand.size() - 1, *m_X_results);

                {
#ifndef COMPETITION

#ifdef TEST
                    if (Verifier::verify(*m_X_results))
#endif
                    {
                        int price = Verifier::calculate_price(*m_X_results);
                        printf("%s: before stream dispath verify:total price is %d\n", __func__, price);
                    }
#ifdef TEST
                    else
                    {
                        printf("%s: solve failed\n", __func__);
                        exit(-1);
                    }
#endif
#endif
                }

                // test_solver(X_results);
                // generate::allocate_flow_to_stream(*m_X_results);
                {
                    std::vector<int> idx_global_demand;
                    for (auto &X : *(m_X_results))
                    {
                        idx_global_demand.push_back(X.idx_global_mtime);
                    }
                    solve::Solver solver(*(m_X_results), std::move(idx_global_demand));
                    solver.solve_stream(50);
                }

                {
#ifdef TEST
                    if (Verifier::verify(*m_X_results) && Verifier::verify_stream(*m_X_results))
#endif
                    {
                        SP_VEC_ANSWER p;
                        if (this->m_X_results_before_dispath_stack.size() > 0)
                        {
                            //因为后续可以从 stack 中取出来，所以不需要拷贝
                            p = m_X_results;
                        }
                        else
                        {
                            p = std::make_shared<std::vector<ANSWER>>(*m_X_results);
                        }
                        m_X_results_after_dispath_queue.put(p);
                    }
#ifdef TEST
                    else
                    {
                        printf("%s: solve failed\n", __func__);
                        exit(-1);
                    }
#endif
                }

                if (MyUtils::Tools::getCurrentMillisecs() - g_start_time > G_TOTAL_DURATION * 1000)
                {
                    m_is_running.store(false);
                    break;
                }

                if (this->m_X_results_before_dispath_stack.size() > 0)
                {
                    this->m_X_results = this->m_X_results_before_dispath_stack.pop();
                }
            }

            //保证 combine 线程正确退出
            m_X_results_after_dispath_queue.put(nullptr);
            // m_X_results_after_dispath_queue.put(nullptr);

            m_thread_pool.waitAll();
            return;
        }

    public:
        static int calculate_price(const std::vector<ANSWER> &X_results,
                                   std::vector<double> &quantile_95_costs)
        {
            std::vector<std::vector<double>> costs(g_num_server, std::vector<double>(X_results.size(), 0));
            for (int t = 0; t < X_results.size(); ++t)
            {
                auto &X = X_results[t];
                for (int id_server = 0; id_server < g_num_server; ++id_server)
                {
                    costs[id_server][t] = X.cost[id_server];
                    // printf("%d,", X.cost[id_server]);
                }
                // printf("\n");
            }

            // quantile_95_costs.resize(g_num_server, 0); //需要保证 quantile_95_costs 的尺寸
            {
                const int m_95_quantile_idx = calculate_quantile_index(0.95, X_results.size());
                auto task = [&costs, &quantile_95_costs, m_95_quantile_idx](int id_server)
                {
                    nth_element(costs[id_server].begin(),
                                costs[id_server].begin() + m_95_quantile_idx,
                                costs[id_server].end());
                    quantile_95_costs[id_server] = costs[id_server][m_95_quantile_idx];
                };
                auto rets = parallel_for(0, g_num_server, task);
                for (auto &i : rets)
                {
                    i.get();
                }
            }
            double sum = 0;
            for (int i = 0; i < g_num_server; i++)
            {
                sum += quantile_95_costs[i];
            }
            return int(sum);
        }

        void combine()
        {
            struct inner_data
            {
                SP_VEC_ANSWER p_X_results;
                std::vector<double> quantile_95_costs;
                int total_price;
                inner_data()
                {
                    p_X_results = std::make_shared<std::vector<ANSWER>>();
                    total_price = INT32_MAX;
                    quantile_95_costs.resize(g_num_server, 0);
                }
                void copy(const inner_data &other)
                {
                    *p_X_results = *other.p_X_results;
                    total_price = other.total_price;
                    // quantile_95_costs = other.quantile_95_costs;
                }
            } data[2], best_data;
            best_data.p_X_results = std::make_shared<std::vector<ANSWER>>();
            best_data.p_X_results->resize(this->m_X_results->size());

            double T = 100, alpha = 0.99;

            while (m_is_running.load())
            {
                data[0].p_X_results = m_X_results_after_dispath_queue.pop();
                while (m_X_results_after_dispath_queue.size() > 0)
                {
                    data[0].p_X_results = m_X_results_after_dispath_queue.pop();
                }
                if (data[0].p_X_results.get() == nullptr)
                {
                    m_is_running.store(false);
                    break;
                }
                {
                    int id0 = 0, id1 = 1;
                    data[id0].total_price = HeuristicAlgorithm::calculate_price(*data[id0].p_X_results, data[id0].quantile_95_costs);
                    cout << "data[0].total_price = " << data[id0].total_price << endl;

                    if (data[id0].total_price < best_data.total_price)
                    {
                        best_data.total_price = data[id0].total_price;
                        *best_data.p_X_results = *data[id0].p_X_results; //需要做一次拷贝
                    }

                    {
                        double delta = data[id1].total_price - data[id0].total_price;

                        //按照概率接受该解
                        double p = exp(-T / delta);
                        if (p > (double)rand() / RAND_MAX)
                        {
                            data[id1].copy(data[id0]);
                            this->m_X_results_before_dispath_stack.put(data[id0].p_X_results);
                        }
                        else
                        {
                            generate::stream_mutation(*data[id0].p_X_results, 0.5, true);
                            this->m_X_results_before_dispath_stack.put(data[id1].p_X_results);
                        }

                        T *= alpha;

                        cout << "delta = " << delta << endl;
                        cout << "T = " << T << endl;
                        cout << "p = " << p << endl;
                    }

                    // generate::stream_mutation(*data[id0].p_X_results, 0.5, true);
                    // data[id0].total_price = HeuristicAlgorithm::calculate_price(*data[id0].p_X_results, data[id0].quantile_95_costs);
                    // srand(time(NULL));
                    // if (data[id0].total_price < best_data.total_price)
                    // {
                    //     best_data.total_price = data[id0].total_price;
                    //     *best_data.p_X_results = *data[id0].p_X_results; //需要做一次拷贝
                    // }
                    // else if (rand() % 100 < 70) //当前解比最优解差
                    // {
                    //     *data[id0].p_X_results = *best_data.p_X_results;
                    // }

                    // this->m_X_results_before_dispath_stack.put(data[id0].p_X_results);

                    std::cout << "******" << __func__ << " " << __LINE__ << ": best_data.total_price is " << best_data.total_price << std::endl;
                }
            }
            printf("%s %d: end\n", __func__, __LINE__);
            m_best_X_results = best_data.p_X_results;
            printf("%s: the last best price is %d\n", __func__, best_data.total_price);
        }
    };

}; // namespace heuristic
