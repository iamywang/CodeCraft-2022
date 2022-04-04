#pragma once

#include "../global.hpp"
#include "../generate/generate.hpp"
#include "../DivideConquer.hpp"

namespace heuristic
{
    class HeuristicAlgorithm
    {
    public:
        std::vector<ANSWER> m_best_results;
        std::vector<ANSWER> &m_X_results;

    public:
        HeuristicAlgorithm(std::vector<ANSWER> &X_results) : m_X_results(X_results) {}
        ~HeuristicAlgorithm() {}

        void optimize()
        {

            int last_price = INT32_MAX;

            for (int i = 0; i < 100; i++)
            {
                DivideConquer::task(0, global::g_demand.client_demand.size() - 1, 500, false, m_X_results);

                {
#ifdef TEST
                    if (Verifier::verify(X_results))
#endif
                    {
                        int price = Verifier::calculate_price(m_X_results);
                        printf("%s: before stream dispath verify:total price is %d\n", __func__, price);
                    }
#ifdef TEST
                    else
                    {
                        printf("%s: solve failed\n", __func__);
                        exit(-1);
                    }
#endif
                }

                // test_solver(X_results);
                generate::allocate_flow_to_stream(m_X_results);

                {
#ifdef TEST
                    if (Verifier::verify(X_results) && Verifier::verify_stream(X_results))
#endif
                    {
                        int price = Verifier::calculate_price(m_X_results);
                        if (last_price > price)
                        {
                            last_price = price;
                            m_best_results = m_X_results;
                            printf("best price is %d\n", last_price);
                        }
                        printf("%s: after stream dispath verify:total price is %d\n", __func__, price);
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
                    break;
                }
            }

            printf("best price is %d\n", last_price);

            return;
        }
    };

}; // namespace heuristic
