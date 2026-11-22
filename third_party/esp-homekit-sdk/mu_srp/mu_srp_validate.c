#include <stdbool.h>
#include <stddef.h>
#include <string.h>

bool mu_srp_valid_public_key(const unsigned char *key, size_t key_len,
                             const unsigned char *modulus, size_t modulus_len)
{
    if (!key || !modulus || !key_len || !modulus_len || key_len > modulus_len)
        return false;
    while (key_len && *key == 0) {
        key++;
        key_len--;
    }
    if (!key_len)
        return false;
    while (modulus_len && *modulus == 0) {
        modulus++;
        modulus_len--;
    }
    if (!modulus_len || key_len > modulus_len)
        return false;
    return key_len < modulus_len || memcmp(key, modulus, key_len) < 0;
}
