#pragma once
#include <Geode/loader/SettingV3.hpp>
#include <Geode/loader/Mod.hpp>
#include "file_utils.hpp"

using namespace geode::prelude;

// Folder Info (custom:folder-info) — read-only display of file counts

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

inline SettingNodeV3* FolderInfoSettingV3::createNode(float width) {
    return FolderInfoSettingNodeV3::create(
        std::static_pointer_cast<FolderInfoSettingV3>(shared_from_this()),
        width
    );
}
