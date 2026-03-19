#pragma once
#include <filesystem>

// All runtime-mutable mod settings grouped in one place.
// Use the inline getters below instead of accessing g_config directly.
struct ModConfig {
    std::filesystem::path songPath;
    std::filesystem::path sfxPath;
    bool separateSfx = false;
};

extern ModConfig g_config;

inline const std::filesystem::path& getSongPath()  { return g_config.songPath; }
inline const std::filesystem::path& getSfxPath()   { return g_config.sfxPath; }
inline bool isSeparateSfxEnabled()                 { return g_config.separateSfx; }
