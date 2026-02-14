//
// Created by Bruno Kuś on 12/02/2026.
//

#ifndef MONGO_2_ENCRYPTIONPROFILE_H
#define MONGO_2_ENCRYPTIONPROFILE_H

enum class EncryptionProfile
{
    AES256_CTR,
    AES192_CTR,
    AES256_GCM,
    AES192_GCM,
    OPE_32_64
};
#endif //MONGO_2_ENCRYPTIONPROFILE_H
