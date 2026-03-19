#include <Geode/Geode.hpp>
#include <Geode/modify/MusicDownloadManager.hpp>
#include "mod_config.hpp"
#include "button_setting.hpp"
#include <unordered_set>

using namespace geode::prelude;

ModConfig g_config;

$on_mod(Loaded) {
    g_config.songPath    = Mod::get()->getSettingValue<std::filesystem::path>("custom-folder");
    g_config.sfxPath     = Mod::get()->getSettingValue<std::filesystem::path>("custom-sfx-folder");
    g_config.separateSfx = Mod::get()->getSettingValue<bool>("separate-sfx-folder");

    log::info("Custom Song Folder loaded. Song: \"{}\", SFX: \"{}\", separate SFX: {}",
        g_config.songPath.string(), g_config.sfxPath.string(), g_config.separateSfx);

    listenForSettingChanges<std::filesystem::path>("custom-folder",
        +[](std::filesystem::path value) {
            g_config.songPath = value;
            log::info("Song folder updated: \"{}\"", value.string());
        }
    );

    listenForSettingChanges<std::filesystem::path>("custom-sfx-folder",
        +[](std::filesystem::path value) {
            g_config.sfxPath = value;
            log::info("SFX folder updated: \"{}\"", value.string());
        }
    );

    listenForSettingChanges<bool>("separate-sfx-folder",
        +[](bool value) {
            g_config.separateSfx = value;
            log::info("Separate SFX folder {}", value ? "enabled" : "disabled");
        }
    );
};

class $modify(MusicDownloadManager) {

    gd::string pathForSongFolder(int p0) {
        if (std::filesystem::exists(getSongPath()))
            return toFolderPath(getSongPath());
        return MusicDownloadManager::pathForSongFolder(p0);
    }

    gd::string pathForSFXFolder(int p0) {
        // When a separate SFX folder is configured and exists, use it
        if (isSeparateSfxEnabled() && std::filesystem::exists(getSfxPath()))
            return toFolderPath(getSfxPath());
        // Otherwise fall back to the shared song directory
        if (std::filesystem::exists(getSongPath()))
            return toFolderPath(getSongPath());
        return MusicDownloadManager::pathForSFXFolder(p0);
    }

    bool customIsSongDownloaded(const std::string& songID) {
        const auto& path = getSongPath();
        if (path.empty()) return false;
        return std::filesystem::exists(path / (songID + ".mp3"))
            || std::filesystem::exists(path / (songID + ".ogg"))
            || std::filesystem::exists(path / (songID + ".wav"));
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

        scanDirIntoStems(getSongPath());
        if (isSeparateSfxEnabled())
            scanDirIntoStems(getSfxPath());

        for (SongInfoObject* info :
             CCArrayExt<SongInfoObject*>(MusicDownloadManager::getDownloadedSongs())) {
            if (customStems.count(std::to_string(info->m_songID)))
                newSongs->addObject(info);
        }

        return newSongs;
    }

};
