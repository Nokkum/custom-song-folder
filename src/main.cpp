#include <Geode/Geode.hpp>
#include <Geode/modify/MusicDownloadManager.hpp>
#include "button_setting.hpp"
#include <unordered_set>

using namespace geode::prelude;

// Store just the path — toFolderPath() adds the trailing separator on demand
std::filesystem::path customPath;

// Separate SFX folder (only used when separate-sfx-folder is enabled)
std::filesystem::path customSfxPath;

bool separateSfxEnabled = false;

$on_mod(Loaded) {
    // Initialize from settings once the mod is fully loaded
    customPath = Mod::get()->getSettingValue<std::filesystem::path>("custom-folder");
    customSfxPath = Mod::get()->getSettingValue<std::filesystem::path>("custom-sfx-folder");
    separateSfxEnabled = Mod::get()->getSettingValue<bool>("separate-sfx-folder");

    listenForSettingChanges<std::filesystem::path>("custom-folder",
        +[](std::filesystem::path value) {
            customPath = value;
        }
    );

    listenForSettingChanges<std::filesystem::path>("custom-sfx-folder",
        +[](std::filesystem::path value) {
            customSfxPath = value;
        }
    );

    listenForSettingChanges<bool>("separate-sfx-folder",
        +[](bool value) {
            separateSfxEnabled = value;
        }
    );
};

class $modify(MusicDownloadManager) {

    gd::string pathForSongFolder(int p0) {
        if (std::filesystem::exists(customPath))
            return toFolderPath(customPath);
        return MusicDownloadManager::pathForSongFolder(p0);
    }

    gd::string pathForSFXFolder(int p0) {
        // When a separate SFX folder is configured and exists, use it
        if (separateSfxEnabled && std::filesystem::exists(customSfxPath))
            return toFolderPath(customSfxPath);
        // Otherwise fall back to the shared song directory
        if (std::filesystem::exists(customPath))
            return toFolderPath(customPath);
        return MusicDownloadManager::pathForSFXFolder(p0);
    }

    bool customIsSongDownloaded(const std::string& songID) {
        if (customPath.empty()) return false;
        return std::filesystem::exists(customPath / (songID + ".mp3"))
            || std::filesystem::exists(customPath / (songID + ".ogg"))
            || std::filesystem::exists(customPath / (songID + ".wav"));
    }

    bool isSongDownloaded(int id) {
        return customIsSongDownloaded(std::to_string(id));
    }

    cocos2d::CCArray* getDownloadedSongs() {
        CCArray* newSongs = CCArray::create();

        // Pre-scan the custom directory once into a set of stems (filename
        // without extension) so we do one directory read instead of two
        // filesystem::exists calls per song entry.
        std::unordered_set<std::string> customStems;
        auto scanDirIntoStems = [&](const std::filesystem::path& dir) {
            if (!std::filesystem::exists(dir)) return;
            std::error_code ec;
            for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
                if (!ec && std::filesystem::is_regular_file(entry.path()))
                    customStems.insert(entry.path().stem().string());
            }
        };

        scanDirIntoStems(customPath);
        if (separateSfxEnabled)
            scanDirIntoStems(customSfxPath);

        for (SongInfoObject* info :
             CCArrayExt<SongInfoObject*>(MusicDownloadManager::getDownloadedSongs())) {
            if (customStems.count(std::to_string(info->m_songID)))
                newSongs->addObject(info);
        }

        return newSongs;
    }

};
