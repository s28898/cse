cmake_minimum_required(VERSION 3.28)
project(mongo_2)

set(CMAKE_CXX_STANDARD 23)

link_directories("/opt/homebrew/Cellar/cryptopp/8.9.0/lib")
link_directories("/opt/homebrew/opt/mongo-cxx-driver/lib")

include_directories("/opt/homebrew/opt/mongo-cxx-driver/include/mongocxx/v_noabi")
include_directories("/opt/homebrew/opt/mongo-cxx-driver/include/bsoncxx/v_noabi")
include_directories("/opt/homebrew/Cellar/nlohmann-json/3.12.0/include")
include_directories("/opt/homebrew/Cellar/cryptopp/8.9.0/include")


add_executable(
        benchmark_generate_db_plain

        benchmark_generate/generate_db_plain.cpp
        bson_util.h
        bson_util.cpp
        benchmark_generate/generate_db_encrypted.cpp
)
add_executable(mongo_2 main.cpp
        EncryptionService_v1.h
        AbstractTest.h
        StatementTransformer.h
        StatementTransformer.cpp
        VariantJsonValue.h
        util.h
        SchemaValidatorTransformer.cpp
        util.cpp
        BsonDecryptor.cpp
        BsonDecryptor.h
        PropertyMetadataStorage.cpp
        PropertyMetadataStorage.h

        ope/aes.hh
        ope/bn.hh
        ope/cmc.hh
        ope/errstream.hh
        ope/hgd.hh
        ope/hgd.cc
        ope/hmac.hh
        ope/ope.cc
        ope/ope.hh
        ope/prng.cc
        ope/prng.hh
        ope/sha.hh
        ope/zz.cc
        ope/zz.hh
        TodoError.cpp
        TodoError.h
        SchemaValidatorTransformer.h
        CommandTransformer.cpp
        CommandTransformer.h
        v2/EncryptionService_v2.cpp
        v2/EncryptionService_v2.h
        AbstractKeyProvider.h
        DummyKeyProvider.cpp
        DummyKeyProvider.h
        MongoDriverSubset.h
        CollectionService.cpp
        cse_service.cpp
        CollectionService.h
        CursorView.cpp
        CursorView.h
        test_v2/CollectionServiceTest.cpp
        test_v2/CollectionServiceTest.h
        v2/StatementTransformerV2.cpp
        v2/StatementTransformerV2.h
        v2/SchemaValidatorTransformerV2.cpp
        v2/SchemaValidatorTransformerV2.h
        v2/BsonDecryptorV2.cpp
        v2/BsonDecryptorV2.h
        KeyPolicy.cpp
        KeyPolicy.h
        EncryptionProfile.h
        v3/EncryptionServiceV3.cpp
        v3/EncryptionServiceV3.h
        v3/StatementTransformerV3.cpp
        v3/StatementTransformerV3.h
        v3/BsonDecryptorV3.cpp
        v3/BsonDecryptorV3.h
        v3/SchemaValidatorTransformerV3.cpp
        v3/SchemaValidatorTransformerV3.h
        v3/CollectionServiceV3.cpp
        v3/CollectionServiceV3.h
        test_v3/CollectionServiceTest.h
)
find_package(nlohmann_json 3.12.0 REQUIRED)

find_package(OpenSSL REQUIRED)
target_link_libraries(mongo_2 PRIVATE OpenSSL::Crypto)
target_link_libraries(mongo_2 PRIVATE "/opt/homebrew/opt/gmp/lib/libgmp.dylib")
target_link_libraries(mongo_2 PRIVATE "/opt/homebrew/Cellar/ntl/11.6.0/lib/libntl.a")

add_executable(
        tests_v2

        v2/EncryptionService_v2.h
        v2/StatementTransformerV2.h
        v2/StatementTransformerV2.cpp
        VariantJsonValue.h
        util.h
        v2/SchemaValidatorTransformerV2.cpp
        util.cpp
        v2/BsonDecryptorV2.cpp
        v2/BsonDecryptorV2.h
        PropertyMetadataStorage.cpp
        PropertyMetadataStorage.h

        ope/aes.hh
        ope/bn.hh
        ope/cmc.hh
        ope/errstream.hh
        ope/hgd.hh
        ope/hgd.cc
        ope/hmac.hh
        ope/ope.cc
        ope/ope.hh
        ope/prng.cc
        ope/prng.hh
        ope/sha.hh
        ope/zz.cc
        ope/zz.hh



        test_v2/doctest_main.cpp
        test_v2/tests/test_and_or_nor_operator.cpp
)

add_executable(
        benchmark_run
        benchmark_run/run.cpp

        v3/BsonDecryptorV3.h
        v3/BsonDecryptorV3.cpp
        v3/CollectionServiceV3.h
        v3/CollectionServiceV3.cpp
        v3/EncryptionServiceV3.h
        v3/EncryptionServiceV3.cpp
        v3/SchemaValidatorTransformerV3.h
        v3/SchemaValidatorTransformerV3.cpp
        v3/StatementTransformerV3.cpp
        v3/StatementTransformerV3.h


        util.h
        util.cpp

        bson_util.h
        bson_util.cpp

        CursorView.h
        CursorView.cpp
        ope/aes.hh
        ope/bn.hh
        ope/cmc.hh
        ope/errstream.hh
        ope/hgd.hh
        ope/hgd.cc
        ope/hmac.hh
        ope/ope.cc
        ope/ope.hh
        ope/prng.cc
        ope/prng.hh
        ope/sha.hh
        ope/zz.cc
        ope/zz.hh
)
add_executable(
        benchmark_generate_db_encrypted

        benchmark_generate/generate_db_encrypted.cpp

        v3/BsonDecryptorV3.h
        v3/BsonDecryptorV3.cpp
        v3/CollectionServiceV3.h
        v3/CollectionServiceV3.cpp
        v3/EncryptionServiceV3.h
        v3/EncryptionServiceV3.cpp
        v3/SchemaValidatorTransformerV3.h
        v3/SchemaValidatorTransformerV3.cpp
        v3/StatementTransformerV3.cpp
        v3/StatementTransformerV3.h


        util.h
        util.cpp

        bson_util.h
        bson_util.cpp

        CursorView.h
        CursorView.cpp
        ope/aes.hh
        ope/bn.hh
        ope/cmc.hh
        ope/errstream.hh
        ope/hgd.hh
        ope/hgd.cc
        ope/hmac.hh
        ope/ope.cc
        ope/ope.hh
        ope/prng.cc
        ope/prng.hh
        ope/sha.hh
        ope/zz.cc
        ope/zz.hh
)
add_executable(
        tests_v3

        ope/aes.hh
        ope/bn.hh
        ope/cmc.hh
        ope/errstream.hh
        ope/hgd.hh
        ope/hgd.cc
        ope/hmac.hh
        ope/ope.cc
        ope/ope.hh
        ope/prng.cc
        ope/prng.hh
        ope/sha.hh
        ope/zz.cc
        ope/zz.hh

        v3/BsonDecryptorV3.h
        v3/BsonDecryptorV3.cpp
        v3/CollectionServiceV3.h
        v3/CollectionServiceV3.cpp
        v3/EncryptionServiceV3.h
        v3/EncryptionServiceV3.cpp
        v3/SchemaValidatorTransformerV3.h
        v3/SchemaValidatorTransformerV3.cpp
        v3/StatementTransformerV3.cpp
        v3/StatementTransformerV3.h

        CursorView.h
        CursorView.cpp

        util.h
        util.cpp

        test_v3/test_and_or_nor_operator.cpp
        test_v3/doctest_main.cpp
        test_v3/test_elemMatch_operator_array.cpp
        test_v3/test_set_operator_dotpath.cpp
        test_v3/test_set_operator.cpp
        test_v3/test_range_predicates.cpp
        test_v3/test_ope_lt.cpp
        test_v3/test_ope_gt.cpp
        test_v3/test_not_operator.cpp
        test_v3/test_nested_documents.cpp
        test_v3/test_nested_arrays.cpp
        test_v3/test_in_operator.cpp
        test_v3/test_in_operator_array.cpp
        test_v3/test_eq_operator.cpp
        test_v3/test_dotpath_array.cpp
        test_v3/test_dotpath.cpp
        bson_util.h
        bson_util.cpp


)
add_executable(tests
        test/doctest_main.cpp
        test/test_1.cpp
        test/test_nested_arrays.cpp
        test/test_ope_simple.cpp
        test/test_nested_documents.cpp

        EncryptionService_v1.h
        AbstractTest.h
        StatementTransformer.h
        StatementTransformer.cpp
        VariantJsonValue.h
        util.h
        SchemaValidatorTransformer.cpp
        util.cpp
        BsonDecryptor.cpp
        BsonDecryptor.h
        PropertyMetadataStorage.cpp
        PropertyMetadataStorage.h

        ope/aes.hh
        ope/bn.hh
        ope/cmc.hh
        ope/errstream.hh
        ope/hgd.hh
        ope/hgd.cc
        ope/hmac.hh
        ope/ope.cc
        ope/ope.hh
        ope/prng.cc
        ope/prng.hh
        ope/sha.hh
        ope/zz.cc
        ope/zz.hh
        test/test_eq_implicit.cpp
        test/test_find_predicates_util.h
        test/test_eq_operator.cpp
        test/test_in_operator.cpp
        test/test_and_or_nor_operator.cpp
        test/test_not_operator.cpp
        test/test_dotpath.cpp
        test/test_elemMatch_operator_array.cpp
        test/test_dotpath_array.cpp
        test/test_in_operator_array.cpp
        test/test_range_predicates.cpp
        test/test_range_predicates_util.h
        test/test_ope_gt.cpp
        SchemaValidatorTransformer.h
        test/test_ope_gte.cpp
        test/test_ope_lt.cpp
        test/test_set_operator.cpp
        CommandTransformer.cpp
        CommandTransformer.h
        test/test_set_operator_dotpath.cpp
        test/test_deletion.cpp
        AbstractKeyProvider.h
        concepts.h
        bson_util.h
        bson_util.cpp
)

target_link_libraries(tests PRIVATE OpenSSL::Crypto)
target_link_libraries(tests PRIVATE "/opt/homebrew/opt/gmp/lib/libgmp.dylib")
target_link_libraries(tests PRIVATE "/opt/homebrew/Cellar/ntl/11.6.0/lib/libntl.a")

target_link_libraries(tests_v2 PRIVATE OpenSSL::Crypto)
target_link_libraries(tests_v2 PRIVATE "/opt/homebrew/opt/gmp/lib/libgmp.dylib")
target_link_libraries(tests_v2 PRIVATE "/opt/homebrew/Cellar/ntl/11.6.0/lib/libntl.a")

target_link_libraries(tests_v3 PRIVATE OpenSSL::Crypto)
target_link_libraries(tests_v3 PRIVATE "/opt/homebrew/opt/gmp/lib/libgmp.dylib")
target_link_libraries(tests_v3 PRIVATE "/opt/homebrew/Cellar/ntl/11.6.0/lib/libntl.a")

target_link_libraries(tests
        PRIVATE
        nlohmann_json::nlohmann_json
        cryptopp
        mongocxx
        bsoncxx
)

target_link_libraries(tests_v2
        PRIVATE
        nlohmann_json::nlohmann_json
        cryptopp
        mongocxx
        bsoncxx
)

target_link_libraries(tests_v3
        PRIVATE
        nlohmann_json::nlohmann_json
        cryptopp
        mongocxx
        bsoncxx
)
target_link_libraries(mongo_2
        PRIVATE
        nlohmann_json::nlohmann_json
        cryptopp
        mongocxx
        bsoncxx
)

target_link_libraries(benchmark_generate_db_plain
        PRIVATE
        nlohmann_json::nlohmann_json
        mongocxx
        bsoncxx
)

target_link_libraries(
        benchmark_generate_db_encrypted
        PRIVATE
        nlohmann_json::nlohmann_json
        mongocxx
        bsoncxx
        cryptopp
)

target_link_libraries(
        benchmark_run
        PRIVATE
        nlohmann_json::nlohmann_json
        mongocxx
        bsoncxx
        cryptopp
)


target_link_libraries(benchmark_generate_db_encrypted PRIVATE OpenSSL::Crypto)
target_link_libraries(benchmark_generate_db_encrypted PRIVATE "/opt/homebrew/opt/gmp/lib/libgmp.dylib")
target_link_libraries(benchmark_generate_db_encrypted PRIVATE "/opt/homebrew/Cellar/ntl/11.6.0/lib/libntl.a")

target_link_libraries(benchmark_run PRIVATE OpenSSL::Crypto)
target_link_libraries(benchmark_run PRIVATE "/opt/homebrew/opt/gmp/lib/libgmp.dylib")
target_link_libraries(benchmark_run PRIVATE "/opt/homebrew/Cellar/ntl/11.6.0/lib/libntl.a")
