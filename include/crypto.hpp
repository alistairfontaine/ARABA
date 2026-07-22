#pragma once

#include <cstdint>
#include <cstring>

namespace araba {

constexpr int AES_BLOCK_SIZE = 16;
constexpr int AES_KEY_SIZE = 32;
constexpr int AES_ROUNDS = 14;

class AES256 {
public:
    explicit AES256(const uint8_t key[AES_KEY_SIZE]);
    void encrypt_block(uint8_t block[AES_BLOCK_SIZE]);
    void decrypt_block(uint8_t block[AES_BLOCK_SIZE]);
    void encrypt_cbc(uint8_t* data, size_t data_len, const uint8_t iv[AES_BLOCK_SIZE]);
    void decrypt_cbc(uint8_t* data, size_t data_len, const uint8_t iv[AES_BLOCK_SIZE]);

private:
    uint8_t round_keys[60][4]; // 60 words of 4 bytes each

    void key_expansion(const uint8_t key[AES_KEY_SIZE]);
    void add_round_key(uint8_t state[AES_BLOCK_SIZE], int round);
    void sub_bytes(uint8_t state[AES_BLOCK_SIZE]);
    void inv_sub_bytes(uint8_t state[AES_BLOCK_SIZE]);
    void shift_rows(uint8_t state[AES_BLOCK_SIZE]);
    void inv_shift_rows(uint8_t state[AES_BLOCK_SIZE]);
    void mix_columns(uint8_t state[AES_BLOCK_SIZE]);
    void inv_mix_columns(uint8_t state[AES_BLOCK_SIZE]);
};

void derive_key(const char* passphrase, uint8_t key[AES_KEY_SIZE]);
void derive_pair_key(const uint8_t mac_a[6], const uint8_t mac_b[6],
                     const uint8_t base_key[AES_KEY_SIZE], uint8_t out_key[AES_KEY_SIZE]);
void generate_iv(uint8_t iv[AES_BLOCK_SIZE]);

} // namespace araba
