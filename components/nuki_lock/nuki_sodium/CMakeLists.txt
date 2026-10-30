# nuki_sodium: the libsodium modules NukiBleEsp32 needs on top of ESPHome's port.
#
# Since ESPHome 2026.10, `api: encryption:` pulls in ESPHome's own libsodium
# (esphome/libsodium) as an ESP-IDF managed component. It can't coexist with
# espressif/libsodium ("Requirement esphome__libsodium and requirement
# espressif__libsodium are both added as project_managed_components"), so
# lock.py always uses ESPHome's port instead. That port ships the full libsodium
# source tree but only compiles what noise needs. This component compiles the
# missing modules from the SAME source tree, so there is exactly one libsodium
# (one version, no duplicate symbols) with and without API encryption.
#
# Nuki uses: crypto_auth_hmacsha256, crypto_box_keypair, crypto_core_hsalsa20,
# crypto_secretbox_easy / crypto_secretbox_open_easy and
# crypto_scalarmult_curve25519 (the latter is already part of the port).
#
# Include paths and -DCONFIGURED=1 come PUBLIC from the libsodium component.

if(CMAKE_BUILD_EARLY_EXPANSION)
    idf_component_register(REQUIRES esphome__libsodium)
    return()
endif()

idf_component_get_property(sodium_dir esphome__libsodium COMPONENT_DIR)
set(S ${sodium_dir}/libsodium/src/libsodium)

idf_component_register(
    SRCS
        # HMAC-SHA256 (SHA-256 itself is part of the port)
        ${S}/crypto_auth/hmacsha256/auth_hmacsha256.c
        # crypto_box_keypair() and what crypto_box.c references
        ${S}/crypto_box/crypto_box.c
        ${S}/crypto_box/curve25519xsalsa20poly1305/box_curve25519xsalsa20poly1305.c
        ${S}/crypto_hash/sha512/hash_sha512.c
        # crypto_secretbox_easy() / crypto_secretbox_open_easy() (XSalsa20-Poly1305)
        ${S}/crypto_secretbox/crypto_secretbox.c
        ${S}/crypto_secretbox/crypto_secretbox_easy.c
        ${S}/crypto_secretbox/xsalsa20poly1305/secretbox_xsalsa20poly1305.c
        ${S}/crypto_stream/xsalsa20/stream_xsalsa20.c
        # crypto_core_hsalsa20() (shared key derivation)
        ${S}/crypto_core/hsalsa20/core_hsalsa20.c
        ${S}/crypto_core/hsalsa20/ref2/core_hsalsa20_ref2.c
    REQUIRES esphome__libsodium
)

# Same warning suppressions as ESPHome's port uses for libsodium sources.
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-unused-function
    -Wno-unknown-pragmas
    -Wno-unused-variable
    -Wno-type-limits
)
