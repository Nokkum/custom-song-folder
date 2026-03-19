#pragma once
#include <Geode/loader/SettingV3.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/utils/file.hpp>

using namespace geode::prelude;

// Open Folder (custom:open-folder) — opens the custom song directory

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
            log::warn("Open Folder: path does not exist — \"{}\"", path.string());
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

inline SettingNodeV3* OpenFolderSettingV3::createNode(float width) {
    return OpenFolderSettingNodeV3::create(
        std::static_pointer_cast<OpenFolderSettingV3>(shared_from_this()),
        width
    );
}
