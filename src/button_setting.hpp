#pragma once
#include "file_utils.hpp"
#include "folder_info_setting.hpp"
#include "open_folder_setting.hpp"
#include "transfer_setting.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// Register all custom setting types
// ─────────────────────────────────────────────────────────────────────────────

$execute {
    (void)Mod::get()->registerCustomSettingType("button",      &MyButtonSettingV3::parse);
    (void)Mod::get()->registerCustomSettingType("open-folder", &OpenFolderSettingV3::parse);
    (void)Mod::get()->registerCustomSettingType("folder-info", &FolderInfoSettingV3::parse);
}
