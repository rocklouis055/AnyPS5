#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"
#include <cstring>

namespace {

constexpr int SCE_OK = 0;
constexpr int SCE_NP_ERROR_INVALID_ARGUMENT = static_cast<int>(0x80550003);
constexpr int SCE_NP_ERROR_INVALID_PLATFORM_TYPE = static_cast<int>(0x80550004);
constexpr int SCE_NP_UTIL_ERROR_NOT_MATCH = static_cast<int>(0x80550609);
constexpr std::size_t ONLINE_ID_MAX_LENGTH = 16;
constexpr std::size_t ONLINE_ID_MIN_LENGTH = 3;

constexpr int PLATFORM_TYPE_NONE = 0;
constexpr int PLATFORM_TYPE_PS3 = 1;
constexpr int PLATFORM_TYPE_VITA = 2;
constexpr int PLATFORM_TYPE_PS4 = 3;

struct CommonNpId {
    NpOnlineId handle;
    std::uint8_t opt[8];
    std::uint8_t reserved[8];
};
static_assert(sizeof(NpOnlineId) == 20 && sizeof(CommonNpId) == 36, "SceNpId is 36 bytes");

int Order(int compare) {
    return compare < 0 ? -1 : compare > 0 ? 1 : 0;
}

int CompareBytes(const std::uint8_t* first, const std::uint8_t* second, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        if (first[i] != second[i]) return first[i] < second[i] ? -1 : 1;
    }
    return 0;
}

bool IsAsciiAlnum(char c) {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

}

extern "C" {

int APS5_VABI sceNpCmpNpIdInOrder(const CommonNpId* np_id1, const CommonNpId* np_id2, std::int32_t* out_result) {
    if (!np_id1 || !np_id2 || !out_result) return SCE_NP_ERROR_INVALID_ARGUMENT;
    int order = Order(std::strncmp(np_id1->handle.data, np_id2->handle.data, ONLINE_ID_MAX_LENGTH));
    if (order == 0) order = CompareBytes(np_id1->opt, np_id2->opt, sizeof(np_id1->opt));
    if (order == 0) order = CompareBytes(np_id1->reserved, np_id2->reserved, sizeof(np_id1->reserved));
    *out_result = order;
    return SCE_OK;
}

int APS5_VABI sceNpCmpOnlineId(const NpOnlineId* online_id1, const NpOnlineId* online_id2) {
    if (!online_id1 || !online_id2) return SCE_NP_ERROR_INVALID_ARGUMENT;
    return std::strncmp(online_id1->data, online_id2->data, ONLINE_ID_MAX_LENGTH) == 0 ? SCE_OK : SCE_NP_UTIL_ERROR_NOT_MATCH;
}

int APS5_VABI sceNpGetPlatformType(const CommonNpId* np_id) {
    if (!np_id) return SCE_NP_ERROR_INVALID_ARGUMENT;
    if (np_id->opt[4] == 0) return PLATFORM_TYPE_NONE;
    const auto* tag = np_id->opt + 4;
    if (std::memcmp(tag, "ps3\0", 4) == 0) return PLATFORM_TYPE_PS3;
    if (std::memcmp(tag, "psp2", 4) == 0) return PLATFORM_TYPE_VITA;
    if (std::memcmp(tag, "ps4\0", 4) == 0) return PLATFORM_TYPE_PS4;
    return SCE_NP_ERROR_INVALID_PLATFORM_TYPE;
}

int APS5_VABI sceNpIntIsValidOnlineId(const NpOnlineId* online_id) {
    if (!online_id || online_id->data[ONLINE_ID_MAX_LENGTH] != 0) return 0;
    const std::size_t length = strnlen(online_id->data, ONLINE_ID_MAX_LENGTH);
    if (length < ONLINE_ID_MIN_LENGTH || !IsAsciiAlnum(online_id->data[0])) return 0;
    for (std::size_t i = 1; i < length; ++i) {
        const char c = online_id->data[i];
        if (!IsAsciiAlnum(c) && c != '_' && c != '-') return 0;
    }
    return 1;
}

}
