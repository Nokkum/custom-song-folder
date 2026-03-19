#pragma once
#include <filesystem>
#include <string>
#include <algorithm>
#include <cctype>
#include <utility>

// Returns the file extension normalized to lowercase (e.g. ".MP3" → ".mp3")
static inline std::string lowerExt(const std::filesystem::path& p) {
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return ext;
}

// Returns true only for user-downloaded songs and SFX, identified by pattern:
//   Songs:  <digits>.mp3 | <digits>.ogg | <digits>.wav
//   SFX:    s<digits>.ogg | s<digits>.wav
// Extension check is case-insensitive to handle files like "12345.MP3".
static bool isUserDownloadedFile(const std::filesystem::path& filepath) {
    std::string stem = filepath.stem().string();
    std::string ext  = lowerExt(filepath);

    if (ext != ".mp3" && ext != ".ogg" && ext != ".wav")
        return false;

    if (!stem.empty() && std::all_of(stem.begin(), stem.end(),
            [](unsigned char c){ return std::isdigit(c); }))
        return true;

    if (stem.size() > 1 && stem[0] == 's' &&
        std::all_of(stem.begin() + 1, stem.end(),
            [](unsigned char c){ return std::isdigit(c); }))
        return true;

    return false;
}

// Returns true for SFX files specifically (s<digits>.*)
static bool isSfxFile(const std::filesystem::path& filepath) {
    std::string stem = filepath.stem().string();
    return stem.size() > 1 && stem[0] == 's' &&
           std::all_of(stem.begin() + 1, stem.end(),
               [](unsigned char c){ return std::isdigit(c); });
}

// Returns a path string guaranteed to end with the platform path separator
static inline std::string toFolderPath(const std::filesystem::path& p) {
    auto s = p.string();
    if (!s.empty() && s.back() != '/' && s.back() != '\\')
        s += static_cast<char>(std::filesystem::path::preferred_separator);
    return s;
}

// Counts songs and SFX in a directory.
// Returns {songCount, sfxCount}.
static std::pair<int,int> countFilesInDir(const std::filesystem::path& dir) {
    int songs = 0, sfx = 0;
    if (!std::filesystem::exists(dir))
        return {0, 0};
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec || !std::filesystem::is_regular_file(entry.path())) continue;
        if (!isUserDownloadedFile(entry.path())) continue;
        if (isSfxFile(entry.path())) ++sfx;
        else ++songs;
    }
    return {songs, sfx};
}
