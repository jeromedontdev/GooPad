#include "goopad_tls.h"

#include "esp_log.h"
#include "esp_random.h"
#include "lwip/inet.h"
#include "mbedtls/oid.h"
#include "mbedtls/pk.h"
#include "mbedtls/x509_crt.h"
#include "nvs.h"
#include "psa/crypto.h"
#include <string.h>

static const char *TAG = "goopad_tls";
static unsigned char s_cert[2048];
static unsigned char s_key[512];

esp_err_t goopad_tls_configure(httpd_ssl_config_t *config, const char *ip)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open("goopad", NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;
    size_t cert_len = sizeof(s_cert), key_len = sizeof(s_key);
    esp_err_t cert_err = nvs_get_str(nvs, "tls_cert", (char *)s_cert, &cert_len);
    esp_err_t key_err = nvs_get_str(nvs, "tls_key", (char *)s_key, &key_len);
    if ((cert_err != ESP_OK && cert_err != ESP_ERR_NVS_NOT_FOUND) ||
        (key_err != ESP_OK && key_err != ESP_ERR_NVS_NOT_FOUND)) {
        nvs_close(nvs);
        return cert_err != ESP_OK ? cert_err : key_err;
    }
    if (cert_err != ESP_OK || key_err != ESP_OK) {
        ESP_LOGI(TAG, "generating unique self-signed P-256 certificate");
        mbedtls_pk_context key;
        mbedtls_x509write_cert cert;
        mbedtls_pk_init(&key);
        mbedtls_x509write_crt_init(&cert);
        psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
        psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
        psa_set_key_bits(&attributes, 256);
        psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_EXPORT);
        psa_set_key_algorithm(&attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
        mbedtls_svc_key_id_t key_id = MBEDTLS_SVC_KEY_ID_INIT;
        int result = psa_crypto_init();
        if (!result) result = psa_generate_key(&attributes, &key_id);
        if (!result) result = mbedtls_pk_copy_from_psa(key_id, &key);
        psa_destroy_key(key_id);
        psa_reset_key_attributes(&attributes);
        if (!result) result = mbedtls_pk_write_key_pem(&key, s_key, sizeof(s_key));
        mbedtls_x509write_crt_set_version(&cert, MBEDTLS_X509_CRT_VERSION_3);
        mbedtls_x509write_crt_set_md_alg(&cert, MBEDTLS_MD_SHA256);
        mbedtls_x509write_crt_set_subject_key(&cert, &key);
        mbedtls_x509write_crt_set_issuer_key(&cert, &key);
        unsigned char serial[16];
        esp_fill_random(serial, sizeof(serial));
        serial[0] = (serial[0] & 0x7f) | 1;
        if (!result) result = mbedtls_x509write_crt_set_serial_raw(&cert, serial, sizeof(serial));
        if (!result) result = mbedtls_x509write_crt_set_subject_name(&cert, "CN=GooPad");
        if (!result) result = mbedtls_x509write_crt_set_issuer_name(&cert, "CN=GooPad");
        if (!result) result = mbedtls_x509write_crt_set_validity(&cert, "20200101000000", "20400101000000");
        if (!result) result = mbedtls_x509write_crt_set_basic_constraints(&cert, 0, -1);
        if (!result) result = mbedtls_x509write_crt_set_key_usage(&cert, MBEDTLS_X509_KU_DIGITAL_SIGNATURE);
        /* DER GeneralNames: DNS goopad.local and this device's initial IPv4 address. */
        unsigned char san[] = {0x30, 20, 0x82, 12, 'g','o','o','p','a','d','.','l','o','c','a','l',
                               0x87, 4, 0, 0, 0, 0};
        struct in_addr address;
        if (!inet_aton(ip, &address)) result = -1;
        else memcpy(san + 18, &address.s_addr, 4);
        if (!result) result = mbedtls_x509write_crt_set_extension(&cert, MBEDTLS_OID_SUBJECT_ALT_NAME,
            MBEDTLS_OID_SIZE(MBEDTLS_OID_SUBJECT_ALT_NAME), 0, san, sizeof(san));
        if (!result) result = mbedtls_x509write_crt_pem(&cert, s_cert, sizeof(s_cert));
        mbedtls_x509write_crt_free(&cert);
        mbedtls_pk_free(&key);
        if (result) {
            ESP_LOGE(TAG, "certificate generation failed: %d", result);
            nvs_close(nvs);
            return ESP_FAIL;
        }
        err = nvs_set_str(nvs, "tls_key", (const char *)s_key);
        if (err == ESP_OK) err = nvs_set_str(nvs, "tls_cert", (const char *)s_cert);
        if (err == ESP_OK) err = nvs_commit(nvs);
    }
    nvs_close(nvs);
    if (err != ESP_OK) return err;
    config->servercert = s_cert;
    config->servercert_len = strlen((const char *)s_cert) + 1;
    config->prvtkey_pem = s_key;
    config->prvtkey_len = strlen((const char *)s_key) + 1;
    return ESP_OK;
}
