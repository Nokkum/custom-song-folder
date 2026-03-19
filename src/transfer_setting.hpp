#pragma once
#include <Geode/loader/SettingV3.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/loader/Loader.hpp>
#include <Geode/ui/Popup.hpp>
#include "file_utils.hpp"
#include <thread>

using namespace geode::prelude;

// Transfer Button (custom:button) — moves files with live progress display

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

    bool load(matjson::Value const&) override { return true; }
    bool save(matjson::Value&) const override { return true; }
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

    // Step 2: validate source, create destinations, then hand off to a background thread
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

        if (!std::filesystem::exists(oldDir)) {
            log::warn("Transfer: source folder does not exist — \"{}\"", oldDir.string());
            FLAlertLayer::create(
                "Migration Status",
                fmt::format("Error: the source folder \"{}\" does not exist.", oldDir.string()),
                "OK"
            )->show();
            return;
        }

        // Ensure destination directories exist, creating them if needed
        std::error_code mkdirEc;
        std::filesystem::create_directories(newDir, mkdirEc);
        if (mkdirEc) {
            log::error("Transfer: could not create song folder \"{}\": {}", newDir.string(), mkdirEc.message());
            FLAlertLayer::create(
                "Migration Status",
                fmt::format("Error: could not create song folder \"{}\":\n{}", newDir.string(), mkdirEc.message()),
                "OK"
            )->show();
            return;
        }

        if (sepSfx) {
            std::filesystem::create_directories(sfxDir, mkdirEc);
            if (mkdirEc) {
                log::error("Transfer: could not create SFX folder \"{}\": {}", sfxDir.string(), mkdirEc.message());
                FLAlertLayer::create(
                    "Migration Status",
                    fmt::format("Error: could not create SFX folder \"{}\":\n{}", sfxDir.string(), mkdirEc.message()),
                    "OK"
                )->show();
                return;
            }
        }

        log::info("Transfer started: \"{}\" → \"{}\"{}",
            oldDir.string(), newDir.string(),
            sepSfx ? fmt::format(" (SFX → \"{}\")", sfxDir.string()) : "");

        // Lock the button and keep this node alive for the duration of the transfer
        m_button->setEnabled(false);
        m_buttonSprite->setString("0 moved...");
        this->retain();

        // Step 3: run file I/O on a background thread to avoid freezing the game
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
                        if (!m_button->isEnabled())
                            m_buttonSprite->setString(
                                fmt::format("{} moved...", snapshot).c_str()
                            );
                    });
                }

                status = moved > 0
                    ? fmt::format("Successfully transferred {} file{}.", moved, moved == 1 ? "" : "s")
                    : "No songs or SFX were found to transfer.";

                log::info("Transfer complete: {} file{} moved.", moved, moved == 1 ? "" : "s");

            } catch (const std::filesystem::filesystem_error& err) {
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

inline SettingNodeV3* MyButtonSettingV3::createNode(float width) {
    return MyButtonSettingNodeV3::create(
        std::static_pointer_cast<MyButtonSettingV3>(shared_from_this()),
        width
    );
}
