//
// Created by Bruno Kuś on 09/02/2026.
//

#ifndef MONGO_2_MONGODRIVERSUBSET_H
#define MONGO_2_MONGODRIVERSUBSET_H

#include <ranges>
#include <type_traits>

#include <bsoncxx/v_noabi/bsoncxx/document/value.hpp>
#include <bsoncxx/v_noabi/bsoncxx/document/view_or_value.hpp>
#include <bsoncxx/v_noabi/bsoncxx/stdx/optional.hpp>

#include <mongocxx/v_noabi/mongocxx/cursor.hpp>
#include <mongocxx/v_noabi/mongocxx/options/find-fwd.hpp>
#include <mongocxx/v_noabi/mongocxx/options/insert-fwd.hpp>
#include <mongocxx/v_noabi/mongocxx/options/update-fwd.hpp>
#include <mongocxx/v_noabi/mongocxx/options/delete-fwd.hpp>
#include <mongocxx/v_noabi/mongocxx/result/insert_one-fwd.hpp>
#include <mongocxx/v_noabi/mongocxx/result/insert_many-fwd.hpp>
#include <mongocxx/v_noabi/mongocxx/result/update-fwd.hpp>
#include <mongocxx/v_noabi/mongocxx/result/delete-fwd.hpp>


template<class T, typename container_type>
concept SupportsInsertMany =
requires(
  T &&t,
  container_type &container,
  const mongocxx::v_noabi::options::insert &options
)
{
  {
  t.insert_many(container, options)
  } -> std::same_as<bsoncxx::v_noabi::stdx::optional<mongocxx::v_noabi::result::insert_many>>;
};

template<class T>
concept SupportsInsertOne =
requires(
  T &&t,
  bsoncxx::v_noabi::document::view_or_value document,
  const mongocxx::v_noabi::options::insert &options
)
{
  {
  t.insert_one(document, options)
  } -> std::same_as<bsoncxx::v_noabi::stdx::optional<mongocxx::v_noabi::result::insert_one>>;
};

template<class T>
concept SupportsFindOne =
requires(
  T &&t,
  bsoncxx::v_noabi::document::view_or_value filter,
  const mongocxx::v_noabi::options::find &options
)
{
  {
  t.find_one(filter, options)
  } -> std::same_as<bsoncxx::v_noabi::stdx::optional<bsoncxx::v_noabi::document::value>>;
};

template<class T>
concept SupportsFind =
requires(
  T &&t,
  bsoncxx::v_noabi::document::view_or_value filter,
  const mongocxx::v_noabi::options::find &options
)
{
  // Allow either the raw driver cursor or any input_range (for middleware adapters).
  requires (
  std::same_as<decltype(t.find(filter, options)), mongocxx::v_noabi::cursor>
  || std::ranges::input_range<decltype(t.find(filter, options))>
  );
};

template<class T>
concept SupportsDeleteOne =
requires(
  T &&t,
  bsoncxx::v_noabi::document::view_or_value filter,
  const mongocxx::v_noabi::options::delete_options &options
)
{
  {
  t.delete_one(filter, options)
  } -> std::same_as<bsoncxx::v_noabi::stdx::optional<mongocxx::v_noabi::result::delete_result>>;
};

template<class T>
concept SupportsDeleteMany =
requires(
  T &&t,
  bsoncxx::v_noabi::document::view_or_value filter,
  const mongocxx::v_noabi::options::delete_options &options
)
{
  {
  t.delete_many(filter, options)
  } -> std::same_as<bsoncxx::v_noabi::stdx::optional<mongocxx::v_noabi::result::delete_result>>;
};

template<class T>
concept SupportsUpdateOne =
requires(
  T &&t,
  bsoncxx::v_noabi::document::view_or_value filter,
  bsoncxx::v_noabi::document::view_or_value update,
  const mongocxx::v_noabi::options::update &options
)
{
  {
  t.update_one(filter, update, options)
  } -> std::same_as<bsoncxx::v_noabi::stdx::optional<mongocxx::v_noabi::result::update>>;
};


template<class T>
concept SupportsUpdateMany =
requires(
  T &&t,
  bsoncxx::v_noabi::document::view_or_value filter,
  bsoncxx::v_noabi::document::view_or_value update,
  const mongocxx::v_noabi::options::update &options
)
{
  {
  t.update_many(filter, update, options)
  } -> std::same_as<bsoncxx::v_noabi::stdx::optional<mongocxx::v_noabi::result::update>>;
};


template<class T>
concept MongoDriverSimpleSubset =
SupportsInsertOne<T> &&
SupportsFindOne<T> &&
SupportsDeleteOne<T> &&
SupportsDeleteMany<T> &&
SupportsUpdateOne<T> &&
SupportsUpdateMany<T>;

template<class T, class Container>
concept MongoDriverSubset =
MongoDriverSimpleSubset<T> &&
SupportsInsertMany<T, Container> &&
SupportsFind<T>;


#endif //MONGO_2_MONGODRIVERSUBSET_H
