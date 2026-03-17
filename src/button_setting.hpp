#pragma once
#include <Geode/loader/SettingV3.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/loader/Dirs.hpp>
#include <Geode/loader/Loader.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/utils/file.hpp>
#include <thread>
#include <algorithm>
#include <cctype>
#include <unordered_set>

using namespace geode::prelude;

// Returns true only for user-downloaded songs and SFX, identified by pattern:
//   Songs:  <digits>.mp3 | <digits>.ogg | <digits>.wav
//   SFX:    s<digits>.ogg | s<digits>.wav
// This replaces the old hardcoded blacklist and is automatically future-proof.
static bool isUserDownloadedFile(const std::filesystem::path& filepath) {
    std::string stem = filepath.stem().string();
    std::string ext  = filepath.extension().string();

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

// Returns true for SFX files specifically (s<digits>.ogg | s<digits>.wav)
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

// ─────────────────────────────────────────────────────────────────────────────
// Folder Info (custom:folder-info) — read-only display of file counts
// ─────────────────────────────────────────────────────────────────────────────

class FolderInfoSettingV3 : public SettingV3 {
public:
    static Result<std::shared_ptr<SettingV3>> parse(
        std::string const& key,
        std::string const& modID,
        matjson::Value const& json
    ) {
        auto res = std::make_shared<FolderInfoSettingV3>();
        auto root = checkJson(json, "FolderInfoSettingV3");
        res->init(key, modID, root);
        res->parseNameAndDescription(root);
        root.checkUnknownKeys();
        return root.ok(std::static_pointer_cast<SettingV3>(res));
    }

    bool load(matjson::Value const&) override { return true; }
    bool save(matjson::Value&) const override { return true; }
    bool isDefaultValue() const override { return true; }
    void reset() override {}

    SettingNodeV3* createNode(float width) override;
};

class FolderInfoSettingNodeV3 : public SettingNodeV3 {
protected:
    CCLabelBMFont* m_countLabel;

    bool init(std::shared_ptr<FolderInfoSettingV3> setting, float width) {
        if (!SettingNodeV3::init(setting, width))
            return false;

        auto path    = Mod::get()->getSettingValue<std::filesystem::path>("custom-folder");
        auto sfxPath = Mod::get()->getSettingValue<std::filesystem::path>("custom-sfx-folder");
        bool sepSfx  = Mod::get()->getSettingValue<bool>("separate-sfx-folder");

        auto [songs, sfx1] = countFilesInDir(path);
        auto [ignore, sfx2] = sepSfx ? countFilesInDir(sfxPath) : std::make_pair(0, 0);
        int sfx = sfx1 + sfx2;

        auto text = fmt::format("{} song{}  |  {} SFX", songs, songs == 1 ? "" : "s", sfx);

        m_countLabel = CCLabelBMFont::create(text.c_str(), "chatFont.fnt");
        m_countLabel->setScale(0.55f);
        m_countLabel->setColor({200, 200, 200});

        this->getButtonMenu()->addChildAtPosition(m_countLabel, Anchor::Center);
        this->getButtonMenu()->updateLayout();

        return true;
    }

    void updateState(CCNode* invoker) override {
        SettingNodeV3::updateState(invoker);
    }

    void onCommit() override {}
    void onResetToDefault() override {}

public:
    static FolderInfoSettingNodeV3* create(
        std::shared_ptr<FolderInfoSettingV3> setting, float width
    ) {
        auto ret = new FolderInfoSettingNodeV3();
        if (ret && ret->init(setting, width)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool hasUncommittedChanges() const override { return false; }
    bool hasNonDefaultValue() const override { return false; }
};

SettingNodeV3* FolderInfoSettingV3::createNode(float width) {
    return FolderInfoSettingNodeV3::create(
        std::static_pointer_cast<FolderInfoSettingV3>(shared_from_this()),
        width
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// Open Folder (custom:open-folder) — opens the custom song directory
// ─────────────────────────────────────────────────────────────────────────────

class OpenFolderSettingV3 : public SettingV3 {
public:
    static Result<std::shared_ptr<SettingV3>> parse(
        std::string const& key,
        std::string const& modID,
        matjson::Value const& json
    ) {
        auto res = std::make_shared<OpenFolderSettingV3>();
        auto root = checkJson(json, "OpenFolderSettingV3");
        res->init(key, modID, root);
        res->parseNameAndDescription(root);
        root.checkUnknownKeys();
        return root.ok(std::static_pointer_cast<SettingV3>(res));
    }

    bool load(matjson::Value const&) override { return true; }
    bool save(matjson::Value&) const override { return true; }
    bool isDefaultValue() const override { return true; }
    void reset() override {}

    SettingNodeV3* createNode(float width) override;
};

class OpenFolderSettingNodeV3 : public SettingNodeV3 {
protected:
    ButtonSprite* m_buttonSprite;
    CCMenuItemSpriteExtra* m_button;

    bool init(std::shared_ptr<OpenFolderSettingV3> setting, float width) {
        if (!SettingNodeV3::init(setting, width))
            return false;

        m_buttonSprite = ButtonSprite::create("Open", "goldFont.fnt", "GJ_button_01.png", .8f);
        m_buttonSprite->setScale(.5f);
        m_button = CCMenuItemSpriteExtra::create(
            m_buttonSprite, this, menu_selector(OpenFolderSettingNodeV3::onButton)
        );

        this->getButtonMenu()->addChildAtPosition(m_button, Anchor::Center);
        this->getButtonMenu()->updateLayout();
        this->updateState(nullptr);

        return true;
    }

    void updateState(CCNode* invoker) override {
        SettingNodeV3::updateState(invoker);
    }

    void onButton(CCObject*) {
        auto path = Mod::get()->getSettingValue<std::filesystem::path>("custom-folder");
        if (!std::filesystem::exists(path)) {
            FLAlertLayer::create(
                "Open Folder",
                fmt::format("The folder <cy>{}</c> does not exist.", path.string()),
                "OK"
            )->show();
            return;
        }
        geode::utils::file::openFolder(path);
    }

    void onCommit() override {}
    void onResetToDefault() override {}

public:
    static OpenFolderSettingNodeV3* create(
        std::shared_ptr<OpenFolderSettingV3> setting, float width
    ) {
        auto ret = new OpenFolderSettingNodeV3();
        if (ret && ret->init(setting, width)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool hasUncommittedChanges() const override { return false; }
    bool hasNonDefaultValue() const override { return false; }
};

SettingNodeV3* OpenFolderSettingV3::createNode(float width) {
    return OpenFolderSettingNodeV3::create(
        std::static_pointer_cast<OpenFolderSettingV3>(shared_from_this()),
        width
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// Transfer Button (custom:button) — moves files with live progress display
// ─────────────────────────────────────────────────────────────────────────────

class MyButtonSettingV3 : public SettingV3 {
public:
    static Result<std::shared_ptr<SettingV3>> parse(
        std::string const& key,
        std::string const& modID,
        matjson::Value const& json
    ) {
        auto res = std::make_shared<MyButtonSettingV3>();
        auto root = checkJson(json, "MyButtonSettingV3");

        res->init(key, modID, root);
        res->parseNameAndDescription(root);
        res->parseEnableIf(root);

        root.checkUnknownKeys();
        return root.ok(std::static_pointer_cast<SettingV3>(res));
    }

    bool load(matjson::Value const& json) override { return true; }
    bool save(matjson::Value& json) const override { return true; }
    bool isDefaultValue() const override { return true; }
    void reset() override {}

    SettingNodeV3* createNode(float width) override;
};

class MyButtonSettingNodeV3 : public SettingNodeV3 {
protected:
    ButtonSprite* m_buttonSprite;
    CCMenuItemSpriteExtra* m_button;

    bool init(std::shared_ptr<MyButtonSettingV3> setting, float width) {
        if (!SettingNodeV3::init(setting, width))
            return false;

        m_buttonSprite = ButtonSprite::create("Transfer", "goldFont.fnt", "GJ_button_01.png", .8f);
        m_buttonSprite->setScale(.5f);
        m_button = CCMenuItemSpriteExtra::create(
            m_buttonSprite, this, menu_selector(MyButtonSettingNodeV3::onButton)
        );

        this->getButtonMenu()->addChildAtPosition(m_button, Anchor::Center);
        this->getButtonMenu()->updateLayout();
        this->updateState(nullptr);

        return true;
    }

    void updateState(CCNode* invoker) override {
        SettingNodeV3::updateState(invoker);
    }

    // Step 1: ask for confirmation before doing anything destructive
    void onButton(CCObject*) {
        createQuickPopup(
            "Transfer Songs",
            "This will <cr>move</c> all songs and SFX from the transfer directory "
            "to your custom folder.\n\nThis <cr>cannot be undone</c>. Continue?",
            "Cancel", "Transfer",
            [this](FLAlertLayer*, bool ok) {
                if (ok) this->startTransfer();
            }
        );
    }

    // Step 2: validate paths, then hand off to a background thread
    void startTransfer() {
        auto oldDir    = Mod::get()->getSettingValue<std::filesystem::path>("migrate-folder");
        auto newDir    = Mod::get()->getSettingValue<std::filesystem::path>("custom-folder");
        auto sfxDir    = Mod::get()->getSettingValue<std::filesystem::path>("custom-sfx-folder");
        bool overwrite = Mod::get()->getSettingValue<bool>("overwrite");
        bool sepSfx    = Mod::get()->getSettingValue<bool>("separate-sfx-folder");

        if (oldDir == newDir) {
            FLAlertLayer::create(
                "Migration Status",
                "No files were transferred because the source and destination directories are the same.",
                "OK"
            )->show();
            return;
        }

        bool srcExists = std::filesystem::exists(oldDir);
        bool dstExists = std::filesystem::exists(newDir);

        if (!srcExists || !dstExists) {
            std::string msg = "Error: the";
            if (!srcExists && !dstExists)
                msg += fmt::format(
                    " folders \"{}\" and \"{}\" do not exist.",
                    oldDir.string(), newDir.string()
                );
            else
                msg += fmt::format(
                    " folder \"{}\" does not exist.",
                    (!srcExists ? oldDir : newDir).string()
                );
            FLAlertLayer::create("Migration Status", msg, "OK")->show();
            return;
        }

        if (sepSfx && !std::filesystem::exists(sfxDir)) {
            FLAlertLayer::create(
                "Migration Status",
                fmt::format("The SFX folder \"{}\" does not exist.", sfxDir.string()),
                "OK"
            )->show();
            return;
        }

        // Lock the button and keep this node alive for the duration of the transfer
        m_button->setEnabled(false);
        m_buttonSprite->setString("0 moved...");
        this->retain();

        // Step 3: run file I/O on a background thread to avoid freezing the game.
        std::thread([this, oldDir, newDir, sfxDir, overwrite, sepSfx]() mutable {
            int moved = 0;
            std::string status;

            try {
                for (const auto& entry : std::filesystem::directory_iterator(oldDir)) {
                    if (!std::filesystem::is_regular_file(entry.path())) continue;
                    if (!isUserDownloadedFile(entry.path())) continue;

                    // Route SFX to the separate SFX folder when that feature is on
                    auto& destDir = (sepSfx && isSfxFile(entry.path())) ? sfxDir : newDir;
                    auto dest     = destDir / entry.path().filename();
                    if (!overwrite && std::filesystem::exists(dest)) continue;

                    // Prefer rename (instant, atomic) but fall back to copy+delete
                    // when source and destination are on different filesystems/drives
                    try {
                        std::filesystem::rename(entry.path(), dest);
                    } catch (const std::filesystem::filesystem_error&) {
                        auto copyOpts = overwrite
                            ? std::filesystem::copy_options::overwrite_existing
                            : std::filesystem::copy_options::none;
                        std::filesystem::copy_file(entry.path(), dest, copyOpts);
                        std::error_code removeEc;
                        std::filesystem::remove(entry.path(), removeEc);
                        if (removeEc)
                            log::warn("Could not remove source file after copy: {}", removeEc.message());
                    }

                    moved++;

                    // Update the button label on the main thread after each file
                    int snapshot = moved;
                    Loader::get()->queueInMainThread([this, snapshot]() {
                        // Guard: button might have been re-enabled (e.g. node destroyed)
                        if (!m_button->isEnabled())
                            m_buttonSprite->setString(
                                fmt::format("{} moved...", snapshot).c_str()
                            );
                    });
                }

                status = moved > 0
                    ? fmt::format("Successfully transferred {} file{}.", moved, moved == 1 ? "" : "s")
                    : "No songs or SFX were found to transfer.";

            } catch (const std::filesystem::filesystem_error& err) {
                // Capture the count before reporting — fixes the old bug where
                // moved was reset to 0 and the partial success was silently lost
                status = fmt::format(
                    "An error occurred while migrating files: {}\n{} file{} transferred before the error.",
                    err.what(), moved, moved == 1 ? " was" : "s were"
                );
                log::error("File migration error: {}", err);
            }

            // Step 4: marshal final result back to the main thread for UI updates
            Loader::get()->queueInMainThread([this, status]() {
                m_button->setEnabled(true);
                m_buttonSprite->setString("Transfer");
                FLAlertLayer::create("Migration Status", status, "OK")->show();
                this->release();
            });
        }).detach();
    }

    void onCommit() override {}
    void onResetToDefault() override {}

public:
    static MyButtonSettingNodeV3* create(
        std::shared_ptr<MyButtonSettingV3> setting, float width
    ) {
        auto ret = new MyButtonSettingNodeV3();
        if (ret && ret->init(setting, width)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool hasUncommittedChanges() const override { return false; }
    bool hasNonDefaultValue() const override { return false; }

    std::shared_ptr<MyButtonSettingV3> getSetting() const {
        return std::static_pointer_cast<MyButtonSettingV3>(SettingNodeV3::getSetting());
    }
};

SettingNodeV3* MyButtonSettingV3::createNode(float width) {
    return MyButtonSettingNodeV3::create(
        std::static_pointer_cast<MyButtonSettingV3>(shared_from_this()),
        width
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// Register all custom setting types
// ─────────────────────────────────────────────────────────────────────────────

$execute {
    (void)Mod::get()->registerCustomSettingType("button",      &MyButtonSettingV3::parse);
    (void)Mod::get()->registerCustomSettingType("open-folder", &OpenFolderSettingV3::parse);
    (void)Mod::get()->registerCustomSettingType("folder-info", &FolderInfoSettingV3::parse);
}
