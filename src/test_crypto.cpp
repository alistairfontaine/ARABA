#include <iostream>
#include <cstring>
#include "crypto.hpp"

using namespace araba;

int main() {
    std::cout << "=== ARABA AES-256 Test ===\n";

    uint8_t key[AES_KEY_SIZE];
    derive_key("ARABA_SECRET_KEY_2026", key);

    uint8_t iv[AES_BLOCK_SIZE];
    generate_iv(iv);

    // Test data (must be multiple of 16)
    char message[] = "Hello ARABA Mesh!!"; // 18 chars, pad to 32
    size_t msg_len = 32; // padded
    uint8_t data[32];
    std::memset(data, 0, 32);
    std::memcpy(data, message, std::strlen(message));

    std::cout << "Original: " << message << "\n";

    AES256 aes(key);
    aes.encrypt_cbc(data, msg_len, iv);

    std::cout << "Encrypted (hex): ";
    for (size_t i = 0; i < msg_len; i++) {
        printf("%02x", data[i]);
    }
    std::cout << "\n";

    aes.decrypt_cbc(data, msg_len, iv);

    std::cout << "Decrypted: " << (char*)data << "\n";

    if (std::memcmp(data, message, std::strlen(message)) == 0) {
        std::cout << "AES-256: PASSED\n";
        return 0;
    } else {
        std::cout << "AES-256: FAILED\n";
        return 1;
    }
}
