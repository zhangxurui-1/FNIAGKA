#include <iomanip>
#include <iostream>
#include <random>
#include <string>

int
main(int argc, char* argv[])
{
    if (argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <base> <delta> <count>" << std::endl;
        return 1;
    }

    double base = std::stod(argv[1]);
    double delta = std::stod(argv[2]);
    int count = std::stoi(argv[3]);
    if (delta < 0)
    {
        std::cerr << "delta must be non-negative" << std::endl;
        return 1;
    }
    if (count < 0)
    {
        std::cerr << "count must be non-negative" << std::endl;
        return 1;
    }

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> sign_dist(0, 1);
    std::uniform_real_distribution<double> offset_dist(0.0, delta);

    std::cout << std::fixed << std::setprecision(3);

    for (int i = 0; i < count; i++)
    {
        double offset = offset_dist(rng);
        double value = base + (sign_dist(rng) == 0 ? -offset : offset);
        if (i > 0)
        {
            std::cout << ", ";
        }
        std::cout << value;
    }
    std::cout << std::endl;
    return 0;
}
