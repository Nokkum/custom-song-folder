# v1.1.2
* Song and SFX files with uppercase extensions (e.g. `.MP3`, `.OGG`) are now recognised correctly
* Destination folders are automatically created during file transfer if they do not already exist, instead of showing an error
* Grouped mod settings into a shared config struct with getter functions for safer access
* File helper utilities moved to a dedicated `file_utils.hpp` header
* Setting types split into individual files for easier maintenance
* Added log messages on mod load, setting changes, and transfer start/complete
* CI pipeline now triggers correctly on pushes to `main`

# v1.1.1
* Fixed mod settings being read before the mod was fully loaded, which could cause the wrong folder to be used on startup
* Fixed the separate SFX folder being ignored in the downloaded songs list
* Fixed Folder Info showing 0 SFX when a separate SFX folder was enabled
* Fixed `isSongDownloaded` checking relative paths when no custom folder was configured yet
* Fixed SFX files being transferred to the songs folder instead of the SFX folder during migration when separate SFX folder was enabled
* Fixed a mid-transfer failure when the SFX folder did not exist — now validated upfront with a clear error message
* Fixed file transfer aborting entirely if a source file could not be deleted after a successful cross-device copy
* Fixed undefined behaviour in file pattern matching on platforms where `char` is signed

# v1.1.0
* Added optional **Separate SFX Folder** — toggle a dedicated directory for SFX independent of songs
* Added **Open Song Folder** button — opens your custom song directory in the OS file explorer
* Added **Folder Info** display — shows a live count of songs and SFX in your custom directory when you open settings
* Transfer button now shows a live progress count (`N moved...`) as files are moved, instead of a static `...` indicator

# v1.0.3
* Added `.wav` file support for songs and SFX
* File transfers now run on a background thread — no more game freeze during large migrations
* Replaced hardcoded file blacklist with pattern-based filtering (future-proof against GD updates)
* Added confirmation dialog before transferring files
* Cross-device file moves now work correctly (different drives/filesystems)
* Fixed partial-success error message not reporting transferred file count
* Optimised downloaded song lookup with a single directory scan instead of per-song file checks
* Fixed typo in "Overwrite Files" setting description
* Code: removed redundant manual path separator logic in favour of `std::filesystem` operators

# v1.0.2
* 2.2081 Compatibility

# v1.0.1
* Fixed an incompatibility with Menu Loop Randomizer
* Code improvements

# v1.0.0
* Release
