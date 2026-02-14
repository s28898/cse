#include "../doctest/doctest.h"
#include "../AbstractTest.h"
#include "test_range_predicates_util.h" // strip_oid


TEST_CASE("testing OPE range queries (int32 domain -> OPE int64)")
{
  OpeSimpleTest test = makeRangePredicatesTest("range_predicates");
  test.createCollection();

  const auto docs = fromFile("../test/data/range_predicates/range_predicates_insert.json");
  test.insertManyFromStatement(docs);



//  SUBCASE("$gt")
//  {
//    run_find_and_expect(test,
//      "../test/data/range_predicates/range_predicates_find_gt.json",
//      R"([
//        { "plain": "c", "num": 30 },
//        { "plain": "d", "num": 40 },
//        { "plain": "e", "num": 50 }
//      ])"
//    );
//  }
//
//  SUBCASE("$gte")
//  {
//    run_find_and_expect(
//      "../test/data/range_predicates/range_predicates_find_gte.json",
//      R"([
//        { "plain": "c", "num": 30 },
//        { "plain": "d", "num": 40 },
//        { "plain": "e", "num": 50 }
//      ])"
//    );
//  }
//
//  SUBCASE("$lt")
//  {
//    run_find_and_expect(
//      "../test/data/range_predicates/range_predicates_find_lt.json",
//      R"([
//        { "plain": "a", "num": 10 },
//        { "plain": "b", "num": 20 }
//      ])"
//    );
//  }
//
//  SUBCASE("$lte")
//  {
//    run_find_and_expect(
//      "../test/data/range_predicates/range_predicates_find_lte.json",
//      R"([
//        { "plain": "a", "num": 10 },
//        { "plain": "b", "num": 20 }
//      ])"
//    );
//  }
//
//  SUBCASE("between (15,45) using $and")
//  {
//    run_find_and_expect(
//      "../test/data/range_predicates/range_predicates_and_or_nor.json",
//      R"([
//        { "plain": "b", "num": 20 },
//        { "plain": "c", "num": 30 },
//        { "plain": "d", "num": 40 }
//      ])"
//    );
//  }
}

TEST_CASE("testing OPE rejects values outside int32 domain")
{
  OpeSimpleTest test = makeRangePredicatesTest("range_predicates");
  test.createCollection();

  SUBCASE("insert rejects num > INT32_MAX")
  {
    const auto docs = fromFile("../test/data/range_predicates/range_predicates_insert_overflow.json");

    CHECK_THROWS(test.insertManyFromStatement(docs));
  }

  SUBCASE("find rejects threshold > INT32_MAX")
  {
    CHECK_THROWS(test.findManyFromFile("../test/data/range_predicates/range_predicates_find_overflow.json"));
  }
}