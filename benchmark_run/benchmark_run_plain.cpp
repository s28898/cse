

// #include "../test_v3/CollectionServiceTest.h"

#include <chrono>
#include <mongocxx/database.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>

#include <bsoncxx/v_noabi/bsoncxx/document/value.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <ratio>

#include <vector>

#include <numeric>
struct Stats
{
  double avg_ms{};
  double median_ms{};
  double min_ms{};
  double max_ms{};
};

auto operator<<(std::ostream& os, const Stats& stats) -> std::ostream&
{
    os << "{ \"avg_ms\": " << stats.avg_ms
    << ", \"median_ms\": " << stats.median_ms
    << ", \"min_ms\": " << stats.min_ms
    << ", \"max_ms\": " << stats.max_ms
    << " }";
    return os;
}

inline
auto compute_stats_ms(std::vector<double> samples) -> Stats
{
  if (samples.empty()) return {};

  std::sort(samples.begin(), samples.end());

  Stats s{};
  s.min_ms = samples.front();
  s.max_ms = samples.back();
  s.avg_ms = std::accumulate(samples.begin(), samples.end(), 0.0) / static_cast<double>(samples.size());

  const auto n = samples.size();
  if (n % 2 == 1) { s.median_ms = samples[n / 2]; }
  else { s.median_ms = 0.5 * (samples[n / 2 - 1] + samples[n / 2]); }

  return s;
}


int main()
{
    constexpr auto collectionName = "collection_plain";
    mongocxx::instance instance{};
    mongocxx::client client{mongocxx::uri{"mongodb://localhost:27091"}};
    mongocxx::database database = client["benchmark_plain"];

    mongocxx::collection collection = database["collection_plain"];

    // teraz chcę wykonać jakieś zapytanie
    // a nawet kilka zapytań
    // i korzystać z chrono czasu dla pomiaru
    const
    auto filter = [] -> bsoncxx::document::value
    {
        using bsoncxx::builder::basic::kvp;
        bsoncxx::builder::basic::document document;
        document.append(kvp("secret", "secret_123"));
        return document.extract();
    }();

    // zacznij liczyć czas
    //
     constexpr auto iterations_count = 100;
    std::vector<double> durations;
    durations.reserve(iterations_count);
    for (int i =0; i < iterations_count; ++i)
    {
        const auto start_time = std::chrono::steady_clock::now();
        auto cursor = collection.find(filter.view());
        const auto after_find_time = std::chrono::steady_clock::now();
        // for (const auto& doc : cursor) { }
        const auto begin = cursor.begin();
        const auto end = cursor.end();
        int count = 0;
        for (auto it = begin; it != end; ++it)
        {
            ++count;
        }
        const auto end_time = std::chrono::steady_clock::now();

        // std::cout << std::chrono::duration<double, std::milli>(end_time - start_time) << '\n';
        // std::cout << std::chrono::duration<double, std::milli>(end_time - after_find_time) << '\n';
        // std::cout << "\"count\"=" << count << '\n';
        const auto duration = std::chrono::duration<double, std::milli>(end_time - start_time);
        durations.push_back(duration.count());
    }
    const auto stats = compute_stats_ms(durations);
    std::cout << "iterations total = " << iterations_count << '\n';
    std::cout << stats << '\n';

    // przestań liczyć czas

}
