//
// Created by Bruno Kuś on 08/02/2026.
//

#ifndef MONGO_2_ABSTRACTKEYPROVIDER_H
#define MONGO_2_ABSTRACTKEYPROVIDER_H
#include <vector>
#include "EncryptionProfile.h"
struct AbstractKeyProvider
{

    
    struct KeyMaterial
    {
        EncryptionProfile profile{};
        std::vector<std::uint8_t> bytes;   // raw key bytes
    };
    
    virtual ~AbstractKeyProvider() = default;
    
    virtual KeyMaterial get_active_key(std::string_view collection, EncryptionProfile profile) = 0;
    virtual KeyMaterial get_key(std::string_view collection, EncryptionProfile profile, std::string_view version) = 0;
//    {
//       Default fallback
//      (void)version;
//      return get_active_key(collection, profile);
//    }
};
#endif //MONGO_2_ABSTRACTKEYPROVIDER_H
