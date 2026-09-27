#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <elf.h>
#include <link.h>
#include <string_view>

namespace levioffhand::runtime::minecraft {

inline constexpr std::string_view kModuleName = "libminecraftpe.so";

enum class Build : std::uint8_t {
    Unsupported = 0,
    V126511,
    V126523,
};

inline constexpr std::array<std::uint8_t, 20> kBuildId126511{
    0x71,0x25,0x09,0xDC,0x14,0xCC,0xC2,0x33,0xE9,0x1F,
    0x26,0x79,0x37,0xDF,0xB4,0x6E,0xCD,0xCC,0x4B,0x68
};

inline constexpr std::array<std::uint8_t, 20> kBuildId126523{
    0x56,0xDE,0x9E,0xED,0x07,0x76,0x31,0xE0,0x3A,0x31,
    0xF4,0xF5,0x8E,0xB2,0xF0,0xE0,0x00,0x71,0xD3,0x35
};

struct ModuleInfo {
    std::uintptr_t base{0};
    Build build{Build::Unsupported};
    std::array<std::uint8_t, 20> buildId{};
    bool found{false};
    bool hasBuildId{false};
};

[[nodiscard]] inline constexpr const char* versionName(Build build) noexcept {
    switch (build) {
        case Build::V126511: return "1.26.51.1";
        case Build::V126523: return "1.26.52.3";
        default: return "unsupported";
    }
}

[[nodiscard]] inline constexpr std::uintptr_t selectRva(
    Build build,
    std::uintptr_t rva126511,
    std::uintptr_t rva126523
) noexcept {
    switch (build) {
        case Build::V126511: return rva126511;
        case Build::V126523: return rva126523;
        default: return 0;
    }
}

namespace detail {

[[nodiscard]] inline constexpr std::uintptr_t align4(
    std::uintptr_t value
) noexcept {
    return (value + 3u) & ~std::uintptr_t{3u};
}

[[nodiscard]] inline std::string_view basenameOf(
    std::string_view path
) noexcept {
    const auto position = path.find_last_of('/');
    return position == std::string_view::npos
        ? path
        : path.substr(position + 1);
}

inline void readBuildId(
    const dl_phdr_info& info,
    const ElfW(Phdr)& header,
    ModuleInfo& module
) noexcept {
    struct NoteHeader {
        std::uint32_t nameSize;
        std::uint32_t descriptorSize;
        std::uint32_t type;
    };

    auto cursor = static_cast<std::uintptr_t>(info.dlpi_addr) +
        static_cast<std::uintptr_t>(header.p_vaddr);
    const auto end = cursor + static_cast<std::uintptr_t>(header.p_memsz);

    while (cursor <= end && sizeof(NoteHeader) <= end - cursor) {
        const auto& note = *reinterpret_cast<const NoteHeader*>(cursor);
        cursor += sizeof(NoteHeader);

        if (note.nameSize > end - cursor) {
            return;
        }
        const auto* name = reinterpret_cast<const char*>(cursor);
        cursor = align4(cursor + note.nameSize);

        if (cursor > end || note.descriptorSize > end - cursor) {
            return;
        }

        if (
            note.type == NT_GNU_BUILD_ID &&
            note.nameSize >= 3 &&
            std::memcmp(name, "GNU", 3) == 0 &&
            note.descriptorSize == module.buildId.size()
        ) {
            std::memcpy(
                module.buildId.data(),
                reinterpret_cast<const void*>(cursor),
                module.buildId.size()
            );
            module.hasBuildId = true;
            if (module.buildId == kBuildId126511) {
                module.build = Build::V126511;
            } else if (module.buildId == kBuildId126523) {
                module.build = Build::V126523;
            }
            return;
        }

        cursor = align4(cursor + note.descriptorSize);
    }
}

struct SearchContext {
    ModuleInfo result{};
};

inline int moduleCallback(
    dl_phdr_info* info,
    std::size_t,
    void* opaque
) noexcept {
    if (info == nullptr) {
        return 0;
    }

    const std::string_view path =
        info->dlpi_name != nullptr ? info->dlpi_name : "";
    if (basenameOf(path) != kModuleName) {
        return 0;
    }

    auto& result = static_cast<SearchContext*>(opaque)->result;
    result.base = static_cast<std::uintptr_t>(info->dlpi_addr);
    result.found = true;

    for (ElfW(Half) index = 0; index < info->dlpi_phnum; ++index) {
        const auto& header = info->dlpi_phdr[index];
        if (header.p_type == PT_NOTE) {
            readBuildId(*info, header, result);
        }
    }
    return 1;
}

} // namespace detail

[[nodiscard]] inline ModuleInfo findLoadedModule() noexcept {
    detail::SearchContext context{};
    dl_iterate_phdr(&detail::moduleCallback, &context);
    return context.result;
}

} // namespace levioffhand::runtime::minecraft
