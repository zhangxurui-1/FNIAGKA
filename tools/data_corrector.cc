#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
std::vector<double>
ParseNumbers(const std::string& text)
{
    std::string normalized;
    normalized.reserve(text.size());
    for (char ch : text)
    {
        switch (ch)
        {
            case '[':
            case ']':
            case ',':
            case ';':
                normalized.push_back(' ');
                break;
            default:
                normalized.push_back(ch);
                break;
        }
    }

    std::stringstream ss(normalized);
    std::vector<double> values;
    double value = 0;
    while (ss >> value)
    {
        values.push_back(value);
    }
    return values;
}

std::vector<double>
ReadValues(int argc, char* argv[], double& strength, int& precision)
{
    std::vector<std::string> payloads;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--strength")
        {
            if (i + 1 >= argc)
            {
                throw std::runtime_error("missing value for --strength");
            }
            strength = std::stod(argv[++i]);
            continue;
        }
        if (arg == "--precision")
        {
            if (i + 1 >= argc)
            {
                throw std::runtime_error("missing value for --precision");
            }
            precision = std::stoi(argv[++i]);
            continue;
        }
        if (arg == "--help" || arg == "-h")
        {
            std::cout << "Usage: " << argv[0]
                      << " [--strength 0.35] [--precision 3] \"1.0, 2.1, 2.9, 4.2\""
                      << std::endl;
            std::cout << "       echo \"1.0 2.1 2.9 4.2\" | " << argv[0] << std::endl;
            std::exit(0);
        }
        payloads.push_back(arg);
    }

    if (strength < 0.0 || strength > 1.0)
    {
        throw std::runtime_error("strength must be in [0, 1]");
    }
    if (precision < 0)
    {
        throw std::runtime_error("precision must be non-negative");
    }

    std::vector<double> values;
    if (!payloads.empty())
    {
        std::string merged;
        for (std::size_t i = 0; i < payloads.size(); ++i)
        {
            if (i > 0)
            {
                merged.push_back(' ');
            }
            merged += payloads[i];
        }
        values = ParseNumbers(merged);
    }
    else
    {
        std::stringstream buffer;
        buffer << std::cin.rdbuf();
        values = ParseNumbers(buffer.str());
    }

    if (values.empty())
    {
        throw std::runtime_error("no numeric values provided");
    }
    return values;
}

std::pair<double, double>
FitLine(const std::vector<double>& values)
{
    if (values.size() == 1)
    {
        return {values.front(), 0.0};
    }

    const double x_mean = static_cast<double>(values.size() - 1) / 2.0;
    double y_mean = 0.0;
    for (double value : values)
    {
        y_mean += value;
    }
    y_mean /= static_cast<double>(values.size());

    double sxx = 0.0;
    double sxy = 0.0;
    for (std::size_t i = 0; i < values.size(); ++i)
    {
        const double dx = static_cast<double>(i) - x_mean;
        sxx += dx * dx;
        sxy += dx * (values[i] - y_mean);
    }

    const double slope = sxy / sxx;
    const double intercept = y_mean - slope * x_mean;
    return {intercept, slope};
}
} // namespace

int
main(int argc, char* argv[])
{
    try
    {
        double strength = 0.35;
        int precision = 3;
        std::vector<double> values = ReadValues(argc, argv, strength, precision);

        const auto [intercept, slope] = FitLine(values);

        std::cout << std::fixed << std::setprecision(precision);
        std::cout << '[';
        for (std::size_t i = 0; i < values.size(); ++i)
        {
            const double fitted = intercept + slope * static_cast<double>(i);
            const double corrected = values[i] + strength * (fitted - values[i]);
            if (i > 0)
            {
                std::cout << ", ";
            }
            std::cout << corrected;
        }
        std::cout << ']' << std::endl;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
