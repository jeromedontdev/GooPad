#pragma once
#include <stdint.h>
#include <stddef.h>
#include <windows.h>
#include <bcrypt.h>

typedef struct { BCRYPT_ALG_HANDLE algorithm; BCRYPT_HASH_HANDLE hash; } psa_hash_operation_t;
#define PSA_HASH_OPERATION_INIT {0}
#define PSA_ALG_SHA_256 1
#define PSA_SUCCESS 0
static unsigned hash_handles;
static int psa_hash_setup(psa_hash_operation_t *op, int algorithm)
{
    (void)algorithm;
    if (BCryptOpenAlgorithmProvider(&op->algorithm, BCRYPT_SHA256_ALGORITHM, NULL, 0) != 0) return -1;
    if (BCryptCreateHash(op->algorithm, &op->hash, NULL, 0, NULL, 0, 0) != 0) {
        BCryptCloseAlgorithmProvider(op->algorithm, 0);
        op->algorithm = NULL;
        return -1;
    }
    hash_handles++;
    return PSA_SUCCESS;
}
static int psa_hash_update(psa_hash_operation_t *op, const uint8_t *data, size_t length)
{
    return BCryptHashData(op->hash, (PUCHAR)data, (ULONG)length, 0) == 0 ? PSA_SUCCESS : -1;
}
static int psa_hash_finish(psa_hash_operation_t *op, uint8_t *out, size_t capacity, size_t *length)
{
    if (capacity < 32 || BCryptFinishHash(op->hash, out, 32, 0) != 0) return -1;
    *length = 32;
    return PSA_SUCCESS;
}
static int psa_hash_abort(psa_hash_operation_t *op)
{
    if (op->hash) { BCryptDestroyHash(op->hash); op->hash = NULL; hash_handles--; }
    if (op->algorithm) { BCryptCloseAlgorithmProvider(op->algorithm, 0); op->algorithm = NULL; }
    return PSA_SUCCESS;
}
