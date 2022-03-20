#include <cstdio>
#include "../utils/tools.hpp"


class CSV
{
public:
    std::vector<std::string> m_header;
    std::vector<std::vector<std::string>> m_content;
    CSV(const std::string &path)
    {
        std::string s;
        if (MyUtils::Tools::readAll(path, s) == -1)
        {
            printf("%s\n", strerror(errno));
        }
        std::vector<std::string> lines = MyUtils::Tools::split(s, "\r\n");

        m_header = MyUtils::Tools::split(lines[0], ",");

        for (auto p = lines.begin() + 1; p != lines.end(); p++)
        {
            std::vector<std::string> line = MyUtils::Tools::split(*p, ",");
            m_content.push_back(line);
        }
    }

    void to_file(const std::string &path)
    {
        std::ofstream ofs(path);

        std::string s;
        for (auto p = m_header.begin(); p != m_header.end(); p++)
        {
            s += *p + ",";
        }
        s.pop_back();
        s += "\r\n";

        ofs << s;

        for (auto p = m_content.begin(); p != m_content.end(); p++)
        {
            s = "";//清空
            for (auto q = p->begin(); q != p->end(); q++)
            {
                s += *q + ",";
            }
            s.pop_back();
            s += "\r\n";
        }

        ofs.flush();
        ofs.close();
    }

    void printHeader()
    {
        for (auto &h : m_header)
        {
            printf("%s ", h.c_str());
        }
        printf("\n");
        return;
    }

    void pritnContent()
    {
        for (auto& c : m_content)
        {
            for (auto& s : c)
            {
                printf("%s ", s.c_str());
            }
            printf("\n");
        }
        return;
    }

    void printAll()
    {
        printHeader();
        pritnContent();
        return;
    }

};
