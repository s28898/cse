#include "../test_v3/CollectionServiceTest.h"

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
    mongocxx::instance instance{};

    constexpr auto uri = "mongodb://localhost:27092";
    constexpr auto databaseName = "benchmark_encrypted";
    constexpr auto collectionName = "collection_encrypted";


  //  const auto validator = json_to_bson_document(jsonFromFile("../benchmark_generate/benchmark_db_plain_validator.json"));
    constexpr auto schemaValidatorExtFilename = "../benchmark_generate/benchmark_db_encrypted_validator.json";
    constexpr auto schemaValidatorTransformedOutputFilename = "../benchmark_generate/output/benchmark_db_encrypted_validator_transformed.json";
    constexpr auto propertyMetadataStorageOutputFilename = "../benchmark_generate/output/benchmark_db_encrypted_property_meta.json";
    constexpr auto propertyMetadataStorageFilename = propertyMetadataStorageOutputFilename;
    constexpr auto validatorTransformedFilename = schemaValidatorTransformedOutputFilename;

    constexpr auto keyPolicyFilename = "../benchmark_generate/benchmark_db_encrypted_key_policy.json";

    processSchemaValidator
      (
        schemaValidatorExtFilename,
        schemaValidatorTransformedOutputFilename,
        propertyMetadataStorageOutputFilename
      );

    DependencyContainer container
      {
        propertyMetadataStorageFilename,
        keyPolicyFilename,
        uri,
        databaseName,
        collectionName
      };
    const auto& collection = *container.collectionServicePtr();
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


    constexpr auto iterations_count = 100;
    std::vector<double> durations;
    durations.reserve(iterations_count);
    for (int i =0; i< iterations_count; ++i)
    {
        // zacznij liczyć czas
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

        const auto duration = std::chrono::duration<double, std::milli>(end_time - start_time);
        durations.push_back(duration.count());
        // std::cout << std::chrono::duration<double, std::milli>(end_time - after_find_time) << '\n';
        // std::cout << "\"count\"=" << count << '\n';
    }

    const auto stats = compute_stats_ms(durations);
    std::cout << "iterations total = " << iterations_count << '\n';
    std::cout << stats << '\n';
    // przestań liczyć czas

}
