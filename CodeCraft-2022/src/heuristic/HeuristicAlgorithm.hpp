#pragma once

#include "../global.hpp"
#include "../generate/generate.hpp"
#include "../DivideConquer.hpp"
#include "../utils/Thread/ThreadSafeContainer.hpp"
#include "../utils/Thread/ThreadPoll.hpp"

namespace heuristic
{
    class HeuristicAlgorithm
    {
    private:
        typedef std::shared_ptr<std::vector<ANSWER>> SP_VEC_ANSWER;

        SP_VEC_ANSWER m_X_results;
        // std::vector<ANSWER>& m_X_results;

        MyUtils::Thread::ThreadSafeQueue<SP_VEC_ANSWER> m_X_results_after_dispath_queue;  //优化之后的解
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

            for (int i = 0; i < 100 && m_is_running.load(); i++)
            {
                DivideConquer::task(0, global::g_demand.client_demand.size() - 1, 500, false, *m_X_results);

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
                generate::allocate_flow_to_stream(*m_X_results);

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
            m_X_results_after_dispath_queue.put(nullptr);

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
                    p_X_results = nullptr;
                    total_price = INT32_MAX;
                    quantile_95_costs.resize(g_num_server, 0);
                }
            } data[2], best_data; // data[2]保存最优解

            auto task = [this, &best_data](inner_data &data_) -> bool
            {
                if (data_.p_X_results.get() == nullptr)
                {
                    return false;
                }

                data_.total_price = HeuristicAlgorithm::calculate_price(*data_.p_X_results, data_.quantile_95_costs);

#ifndef COMPETITION
                printf("%s %d: after stream dispath total price is %d\n", __FILE__, __LINE__, data_.total_price);
#endif

                if (data_.total_price < best_data.total_price)
                {
                    // best_data.total_price = data_.total_price;
                    // best_data.p_X_results = data_.p_X_results;
                    // best_data.quantile_95_costs = data_.quantile_95_costs;
                    best_data = data_;
#ifndef COMPETITION
                    printf("%s %d: best price is %d\n", __func__, __LINE__, best_data.total_price);
#endif
                }
                return true;
            };

            while (m_is_running.load())
            {
                data[0].p_X_results = m_X_results_after_dispath_queue.pop();

                if (!task(data[0]))
                {
                    m_is_running.store(false);
                    break;
                }

                data[1].p_X_results = m_X_results_after_dispath_queue.pop();
                if (!task(data[1]))
                {
                    m_is_running.store(false);
                    break;
                }

                int id0 = 0, id1 = 1;
                if (data[0].total_price > data[1].total_price) //保证id0指向的是小的
                {
                    id0 = 1, id1 = 0;
                }

                {
                    bool swap_flag_X0_X1 = false;
                    bool swap_flag_X01_best_data = false;
                    for (int t = 0; t < data[0].p_X_results->size(); t++)
                    {
                        bool flag_X0_X1 = true;
                        bool flag_X01_best = true;
                        auto &X0 = (*data[id0].p_X_results)[t];
                        auto &X1 = (*data[id1].p_X_results)[t];
                        auto &best_X = (*best_data.p_X_results)[t];

                        for (int id_server = 0; id_server < g_num_server; ++id_server)
                        {
                            if (X1.cost[id_server] > data[id0].quantile_95_costs[id_server] &&
                                X0.cost[id_server] < data[id0].quantile_95_costs[id_server])
                            {
                                flag_X0_X1 = false;
                                break;
                            }
                            // if(X0.cost[id_server] > best_data.quantile_95_costs[id_server] &&
                            //    best_X.cost[id_server] < best_data.quantile_95_costs[id_server])
                            // {
                            //     flag_X01_best = false;
                            //     break;
                            // }
                        }
                        if (flag_X0_X1)
                        {
                            ANSWER::swap(X0, X1);
                            swap_flag_X0_X1 = true;
                        }
                        // if(flag_X01_best)
                        // {
                        //     ANSWER::swap(best_X, X0);
                        //     swap_flag_X01_best_data = true;
                        // }
                    }

                    // if(swap_flag_X01_best_data)
                    // {
                    //     best_data.total_price = HeuristicAlgorithm::calculate_price(*best_data.p_X_results, best_data.quantile_95_costs);
                    // }

                    if (swap_flag_X0_X1)
                    {

                        data[id0].total_price = HeuristicAlgorithm::calculate_price(*data[id0].p_X_results, data[id0].quantile_95_costs);
                        printf("after swap, %s: data[id0].total_price is %d\n", __func__,data[id0].total_price);


                        if (data[id0].total_price < best_data.total_price)
                        {
                            best_data.total_price = data[id0].total_price;
                            *best_data.p_X_results = *data[id0].p_X_results; //需要做一次拷贝

                            printf("%s %d: best price is %d\n", __func__, __LINE__,best_data.total_price);
                        }

                        this->m_X_results_before_dispath_stack.put(data[id0].p_X_results);
                        srand(time(NULL));
                        if(rand() % 100 < 30)//50%的几率
                        {
                            this->m_X_results_before_dispath_stack.put(data[id1].p_X_results);
                        }
                    }
                }
            }
            printf("%s %d: end\n", __func__, __LINE__);
            m_best_X_results = best_data.p_X_results;
            printf("%s: the last best price is %d\n", __func__, best_data.total_price);
        }
    };

}; // namespace heuristic
