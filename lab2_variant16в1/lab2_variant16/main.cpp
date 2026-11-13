// Програмний проєкт №2, варіант 16
// Kondtraenko Yaroslav 

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <execution>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace std;

using Clock = chrono::steady_clock;
using Sequence = vector<long long>;
using Iterator = Sequence::const_iterator;

const int TESTS = 5;
const size_t MAX_K = 32;
const vector<size_t> SIZES = { 100000, 500000, 1000000 };

struct Measurement
{
    size_t k;
    double time;
};


string dataFileName(size_t size)
{
    return "data/data_" + to_string(size) + ".txt";
}

void saveData(const string& fileName, size_t size)
{
    mt19937 generator(12345 + static_cast<unsigned int>(size));
    uniform_int_distribution<int> distribution(-1000000, 1000000);

    ofstream file(fileName);

    for (size_t i = 0; i < size; ++i)
        file << distribution(generator) << '\n';
}

Sequence loadData(const string& fileName)
{
    ifstream file(fileName);
    Sequence data;
    long long value;

    while (file >> value)
        data.push_back(value);

    return data;
}

Sequence loadOrCreateData(size_t size)
{
    string fileName = dataFileName(size);
    Sequence data = loadData(fileName);

    if (data.size() != size)
    {
        saveData(fileName, size);
        cout << "Generated: " << fileName << '\n';
        data = loadData(fileName);
    }

    return data;
}


bool lessByAbs(long long a, long long b)
{
    return llabs(a) < llabs(b);
}

long long maxAbs(Sequence::const_iterator first, Sequence::const_iterator last)
{
    return llabs(*max_element(first, last, lessByAbs));
}

template <class... Policy>
long long maxDifference(Iterator first, Iterator last, Policy... policy)
{
    if (last - first < 2)
        return 0;

    Sequence differences(last - first);
    adjacent_difference(policy..., first, last, differences.begin());

    return maxAbs(differences.begin() + 1, differences.end());
}

long long parallelMaxDifference(const Sequence& data, size_t k)
{
    if (data.size() < 2)
        return 0;

    size_t pairs = data.size() - 1;
    k = min(k, pairs);

    Sequence results(k, 0);
    vector<thread> threads;
    threads.reserve(k);

    size_t partSize = pairs / k;
    size_t extra = pairs % k;
    size_t firstPair = 1;

    for (size_t i = 0; i < k; ++i)
    {
        size_t count = partSize + (i < extra ? 1 : 0);
        size_t endPair = firstPair + count;

        Iterator first = data.begin() + static_cast<ptrdiff_t>(firstPair - 1);
        Iterator last = data.begin() + static_cast<ptrdiff_t>(endPair);

        threads.emplace_back([&results, i, first, last]()
        {
            results[i] = maxDifference(first, last);
        });

        firstPair = endPair;
    }

    for (thread& t : threads)
        t.join();

    return *max_element(results.begin(), results.end());
}


template <class Function>
double measure(Function operation)
{
    chrono::duration<double, milli> total(0);

    for (int i = 0; i < TESTS; ++i)
    {
        auto start = Clock::now();
        volatile long long result = operation();
        auto finish = Clock::now();

        total += finish - start;
        (void)result;
    }

    return total.count() / TESTS;
}

template <class... Policy>
void printLibraryTime(const string& title, const Sequence& data,
                      Policy... policy)
{
    double time = measure([&]()
    {
        return maxDifference(data.begin(), data.end(), policy...);
    });

    cout << left << setw(16) << title << time << " ms\n";
}

void testLibrary(const Sequence& data)
{
    cout << "\nLibrary algorithms:\n";

    printLibraryTime("No policy:", data);
    printLibraryTime("execution::seq:", data, execution::seq);
    printLibraryTime("execution::par:", data, execution::par);
    printLibraryTime("par_unseq:", data, execution::par_unseq);
}

size_t hardwareThreadCount()
{
    size_t count = thread::hardware_concurrency();
    return count == 0 ? 1 : count;
}

vector<Measurement> testOwnAlgorithm(const Sequence& data, long long expected,
                                     size_t maxK)
{
    cout << "\nOwn parallel algorithm:\n";
    cout << left << setw(10) << "K" << setw(18) << "Time (ms)" << "Correct\n";

    vector<Measurement> times;

    for (size_t k = 1; k <= maxK; ++k)
    {
        long long result = 0;

        double time = measure([&]()
        {
            result = parallelMaxDifference(data, k);
            return result;
        });

        cout << left << setw(10) << k << setw(18) << time
             << (result == expected ? "yes" : "NO") << '\n';

        times.push_back({ k, time });
    }

    return times;
}

void printBestK(const vector<Measurement>& times, size_t hardwareThreads)
{
    auto best = min_element(times.begin(), times.end(),
        [](const Measurement& a, const Measurement& b)
        {
            return a.time < b.time;
        });

    cout << "Hardware threads: " << hardwareThreads << '\n';
    cout << "Best K: " << best->k << ", time: " << best->time << " ms\n";
    cout << "Best K / hardware threads: "
         << static_cast<double>(best->k) / hardwareThreads << '\n';
}

void testData(const Sequence& data)
{
    cout << "\nData size: " << data.size() << '\n';

    long long expected = maxDifference(data.begin(), data.end());

    cout << "Maximum absolute difference: " << expected << '\n';
    cout << fixed << setprecision(4);

    testLibrary(data);

    size_t hardwareThreads = hardwareThreadCount();
    size_t maxK = min({ MAX_K, hardwareThreads * 2, data.size() - 1 });

    printBestK(testOwnAlgorithm(data, expected, maxK), hardwareThreads);
}

int main()
{
    cout << "Variant 16 - adjacent_difference\n";
    cout << "Maximum absolute difference between neighbours\n";
    cout << "Compiler: MSVC, C++20. Configuration: Release.\n";

    filesystem::create_directories("data");

    for (size_t size : SIZES)
        testData(loadOrCreateData(size));

    cout << "\nAll tests finished.\n";
    return 0;
}
