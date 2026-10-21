#include "prx/libc/include/general/VabiMacros.hpp"
#include "SceTypes.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

struct CommonNpId {
    NpOnlineId handle;
    std::uint8_t opt[8];
    std::uint8_t reserved[8];
};

}

extern "C" {
int APS5_VABI sceNpCmpNpIdInOrder(const CommonNpId* np_id1, const CommonNpId* np_id2, std::int32_t* out_result);
int APS5_VABI sceNpCmpOnlineId(const NpOnlineId* online_id1, const NpOnlineId* online_id2);
int APS5_VABI sceNpGetPlatformType(const CommonNpId* np_id);
int APS5_VABI sceNpIntIsValidOnlineId(const NpOnlineId* online_id);
}

namespace {

constexpr int InvalidArgument = static_cast<int>(0x80550003u);
constexpr int InvalidPlatformType = static_cast<int>(0x80550004u);
constexpr int NotMatch = static_cast<int>(0x80550609u);

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "NpCommon check failed: %s\n", what);
        std::abort();
    }
}

NpOnlineId Online(const char* text) {
    NpOnlineId id{};
    std::memcpy(id.data, text, std::strlen(text));
    return id;
}

CommonNpId Np(const char* text, const char* platform = "") {
    CommonNpId id{};
    id.handle = Online(text);
    std::memcpy(id.opt + 4, platform, std::strlen(platform));
    return id;
}

int Order(const CommonNpId& first, const CommonNpId& second) {
    std::int32_t result = 7;
    Require(sceNpCmpNpIdInOrder(&first, &second, &result) == 0, "compare in order");
    return result;
}

void CheckCompareInOrder() {
    const auto alpha = Np("alpha");
    std::int32_t result = 0;
    Require(sceNpCmpNpIdInOrder(nullptr, &alpha, &result) == InvalidArgument, "in order first null");
    Require(sceNpCmpNpIdInOrder(&alpha, nullptr, &result) == InvalidArgument, "in order second null");
    Require(sceNpCmpNpIdInOrder(&alpha, &alpha, nullptr) == InvalidArgument, "in order result null");
    Require(Order(alpha, Np("alpha")) == 0, "equal ids");
    Require(Order(alpha, Np("beta")) == -1, "online id orders first");
    Require(Order(Np("beta"), alpha) == 1, "online id orders second");
    Require(Order(Np("alpha", "ps3"), Np("alpha", "ps4")) == -1, "opt orders when online ids match");
    auto high = alpha;
    high.reserved[7] = 0xff;
    Require(Order(high, alpha) == 1, "reserved bytes compare unsigned");
    auto bytes = alpha;
    bytes.handle.data[0] = static_cast<char>(0xe0);
    Require(Order(bytes, alpha) == 1, "online id bytes compare unsigned");
}

void CheckCompareOnlineId() {
    const auto first = Online("player_1");
    auto second = Online("player_1");
    Require(sceNpCmpOnlineId(nullptr, &second) == InvalidArgument, "online id first null");
    Require(sceNpCmpOnlineId(&first, nullptr) == InvalidArgument, "online id second null");
    Require(sceNpCmpOnlineId(&first, &second) == 0, "matching online ids");
    second.pad[0] = 1;
    Require(sceNpCmpOnlineId(&first, &second) == 0, "bytes after the terminator are ignored");
    const auto other = Online("player_2");
    Require(sceNpCmpOnlineId(&first, &other) == NotMatch, "different online ids");
}

void CheckPlatformType() {
    Require(sceNpGetPlatformType(nullptr) == InvalidArgument, "platform null");
    const auto none = Np("someone");
    Require(sceNpGetPlatformType(&none) == 0, "no platform");
    const auto ps3 = Np("someone", "ps3");
    const auto vita = Np("someone", "psp2");
    const auto ps4 = Np("someone", "ps4");
    const auto other = Np("someone", "ps5");
    Require(sceNpGetPlatformType(&ps3) == 1, "ps3 platform");
    Require(sceNpGetPlatformType(&vita) == 2, "vita platform");
    Require(sceNpGetPlatformType(&ps4) == 3, "ps4 platform");
    Require(sceNpGetPlatformType(&other) == InvalidPlatformType, "unknown platform");
}

bool Valid(const char* text) {
    const auto id = Online(text);
    return sceNpIntIsValidOnlineId(&id) == 1;
}

void CheckValidOnlineId() {
    Require(sceNpIntIsValidOnlineId(nullptr) == 0, "valid null");
    Require(Valid("abc"), "three characters");
    Require(Valid("Player_1-x"), "letters digits underscore and hyphen");
    Require(Valid("0123456789abcdef"), "sixteen characters");
    Require(!Valid("ab"), "too short");
    Require(!Valid("_player"), "starts with underscore");
    Require(!Valid("-player"), "starts with hyphen");
    Require(!Valid("bad.name"), "dot");
    Require(!Valid("bad name"), "space");
    auto unterminated = Online("0123456789abcdef");
    unterminated.data[16] = 'x';
    Require(sceNpIntIsValidOnlineId(&unterminated) == 0, "no terminator");
    auto high = Online("abc");
    high.data[1] = static_cast<char>(0xe9);
    Require(sceNpIntIsValidOnlineId(&high) == 0, "non-ASCII byte");
}

}

int main() {
    CheckCompareInOrder();
    CheckCompareOnlineId();
    CheckPlatformType();
    CheckValidOnlineId();
}
