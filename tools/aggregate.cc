#include <algorithm>
#include <chrono>
#include <ctime>
#include <dirent.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace
{
bool
IsRegularFile(const std::string& path)
{
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

std::string
JoinPath(const std::string& dir, const std::string& file)
{
    if (dir.empty() || dir.back() == '/')
    {
        return dir + file;
    }
    return dir + "/" + file;
}

std::string
BaseName(const std::string& path)
{
    auto pos = path.find_last_of('/');
    if (pos == std::string::npos)
    {
        return path;
    }
    return path.substr(pos + 1);
}
} // namespace

std::string
NowToString()
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
    localtime_r(&t, &tm);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d-%H%M%S");
    return oss.str();
}

int
main(int argc, char* argv[])
{
    std::string dir = ".";
    if (argc > 1)
    {
        dir = argv[1];
    }

    std::string out_file = JoinPath(dir, "exp_data_summary_" + NowToString() + ".log");

    std::map<std::string, std::vector<double>> avg_datas;
    std::vector<std::string> files;

    DIR* dp = opendir(dir.c_str());
    if (!dp)
    {
        std::cerr << "Failed to open directory: " << dir << std::endl;
        return 1;
    }

    while (dirent* entry = readdir(dp))
    {
        std::string filename = entry->d_name;
        std::string full_path = JoinPath(dir, filename);
        if (filename.rfind("exp_security", 0) == 0 && IsRegularFile(full_path))
        {
            files.push_back(full_path);
        }
    }
    closedir(dp);

    auto extract_security = [](const std::string& path) {
        auto s = BaseName(path);
        auto pos = s.find("security");
        int l = 0;
        while (pos + 8 + l < s.size() && s[pos + 8 + l] >= '0' && s[pos + 8 + l] <= '9')
        {
            ++l;
        }
        return std::stoi(s.substr(pos + 8, l));
    };

    auto extract_size = [](const std::string& path) {
        auto s = BaseName(path);
        auto pos = s.find("size");
        int l = 0;
        while (pos + 4 + l < s.size() && s[pos + 4 + l] >= '0' && s[pos + 4 + l] <= '9')
        {
            ++l;
        }
        return std::stoi(s.substr(pos + 4, l));
    };

    std::sort(files.begin(), files.end(), [&](const std::string& a, const std::string& b) {
        int sa = extract_security(a);
        int sb = extract_security(b);
        if (sa != sb)
        {
            return sa < sb;
        }
        return extract_size(a) < extract_size(b);
    });

    for (int i = 0; i < files.size(); i++)
    {
        auto& file = files[i];
        int security_level = extract_security(file);
        std::string event_name;
        std::cout << "Processing file: " << file << std::endl;
        std::ifstream infs(file);
        std::string line;
        while (std::getline(infs, line))
        {
            auto pos = line.find_first_not_of(" \t");
            if (pos == std::string::npos || line[pos] == '+')
            {
                continue;
            }
            std::string subline = line.substr(pos);
            if (subline.size() >= 6 && subline.substr(subline.size() - 6) == "events")
            {
                event_name = subline.substr(0, subline.find_first_of(':'));
            }
            else if (subline.size() >= 3 && subline.substr(0, 3) == "Avg")
            {
                double avg = std::stod(subline.substr(4, subline.find_first_of(' ') - 4));
                std::string key = event_name + " (" + std::to_string(security_level) + " bit)";
                avg_datas[key].push_back(avg / 1000);
            }
        }
    }

    std::ofstream outfs(out_file);
    for (const auto& [event_name, avgs] : avg_datas)
    {
        outfs << event_name << ":[";
        for (auto it=avgs.begin();it!=avgs.end();it++){
            if(it-avgs.begin()<avgs.size()-1){
                outfs<<*it<<", ";
            }else{
                outfs<<*it;
            }
        }
        outfs << "]" << std::endl;
    }

    return 0;
}
