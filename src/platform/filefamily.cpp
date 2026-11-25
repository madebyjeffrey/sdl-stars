#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include "filefamily.h"
#include "fileio.h"
#include "path_utf8.h"

namespace fs = std::filesystem;
namespace {
std::string Text(const fs::path &path) { return SzFromUtf8(path.u8string()); }
void        Require(bool f) {
    if (!f)
        throw std::runtime_error("game family transaction failed");
}
std::string Fold(std::string sz) {
    for (auto &ch : sz)
        if (ch >= 'A' && ch <= 'Z')
            ch += 'a' - 'A';
    return sz;
}
bool Extension(std::string sz) {
    sz = Fold(std::move(sz));
    if (sz == ".xy" || sz == ".hst")
        return true;
    if (sz.size() < 3 || sz.size() > 4 || sz[0] != '.' || (sz[1] != 'm' && sz[1] != 'x' && sz[1] != 'h'))
        return false;
    if (sz[2] < '1' || sz[2] > '9')
        return false;
    if (sz.size() == 3)
        return true;
    return sz[2] == '1' && sz[3] >= '0' && sz[3] <= '6';
}
bool Backup(std::string_view sz) {
    if (!sz.starts_with("backup"))
        return false;
    sz.remove_prefix(6);
    const bool fNumbered = sz.starts_with('.');
    if (fNumbered) sz.remove_prefix(1);
    return sz.size() <= (fNumbered ? 5u : 3u) && std::all_of(sz.begin(), sz.end(), [](char c) { return c >= '0' && c <= '9'; });
}
std::set<fs::path> Members(const fs::path &root, const fs::path &stem) {
    std::set<fs::path> result;
    auto               scan = [&](const fs::path &dir) {
        for (const auto &entry : fs::directory_iterator(dir)) {
            if (Fold(Text(entry.path().stem())) != Fold(Text(stem)) || !Extension(Text(entry.path().extension())))
                continue;
            Require(entry.symlink_status().type() == fs::file_type::regular);
            result.insert(entry.path().lexically_relative(root));
        }
    };
    scan(root);
    for (const auto &entry : fs::directory_iterator(root)) {
        if (!Backup(Fold(Text(entry.path().filename()))))
            continue;
        Require(!entry.is_symlink());
        if (entry.is_directory())
            scan(entry.path());
    }
    return result;
}
void Sync(const fs::path &path) { Require(ISyncNativeDirectory(Text(path)) == 0); }
void Copy(const fs::path &source, const fs::path &target) {
    fs::create_directories(target.parent_path());
    NATIVEFILEOWNER input(PfNativeOpen(Text(source), NATIVEFILEMODE::read), ICloseNativeFile);
    auto            output = NftNativeCreateTemp(Text(target.parent_path() / ".stars-copy-XXXXXX"));
    Require(input && output.lpfile);
    try {
        std::array<std::byte, 65536> rgb;
        for (;;) {
            const auto cb = CbReadNativeFile(input.get(), rgb);
            Require(cb.has_value());
            if (*cb == 0)
                break;
            Require(CbWriteNativeFile(output.lpfile.get(), std::span(rgb).first(*cb)) == cb);
        }
        Require(ICopyNativePermissions(output.lpfile.get(), Text(source)) == 0);
        Require(ISyncNativeFile(output.lpfile.get()) == 0);
        output.lpfile.reset();
        Require(IRenameNativePath(output.szPath, Text(target), true, true) == 0);
    } catch (...) {
        output.lpfile.reset();
        IRemoveNativePath(output.szPath, false);
        throw;
    }
}
bool SameBytes(const fs::path &a, const fs::path &b) {
    if (!fs::is_regular_file(b) || fs::file_size(a) != fs::file_size(b)) return false;
    if (fs::equivalent(a, b)) return true;
    NATIVEFILEOWNER first(PfNativeOpen(Text(a), NATIVEFILEMODE::read), ICloseNativeFile);
    NATIVEFILEOWNER second(PfNativeOpen(Text(b), NATIVEFILEMODE::read), ICloseNativeFile);
    Require(first && second);
    std::array<std::byte, 16384> left, right;
    for (;;) {
        const auto cb = CbReadNativeFile(first.get(), left);
        Require(cb.has_value() && CbReadNativeFile(second.get(), right) == cb);
        if (*cb == 0) return true;
        if (!std::equal(left.begin(), left.begin() + static_cast<std::ptrdiff_t>(*cb), right.begin())) return false;
    }
}

void Marker(const fs::path &path) {
    auto file = NftNativeCreateTemp(Text(path) + ".XXXXXX");
    Require(bool(file.lpfile));
    bool ok = IPutNativeString("sdl-stars-family-1\n", file.lpfile.get()) == 0 && ISyncNativeFile(file.lpfile.get()) == 0;
    file.lpfile.reset();
    if (ok)
        ok = IRenameNativePath(file.szPath, Text(path), true, true) == 0;
    if (!ok)
        IRemoveNativePath(file.szPath, false);
    Require(ok);
    Sync(path.parent_path());
}
} // namespace
struct NATIVEFAMILYSTATE {
    fs::path        base, parent, journal;
    NATIVEFILEOWNER lock{nullptr, ICloseNativeFile};

    void            Cleanup() {
        fs::remove_all(journal);
        Sync(journal.parent_path());
    }
    void Recover() {
        if (!fs::exists(journal))
            return;
        Require(!fs::is_symlink(journal));
        if (fs::exists(journal / "ready") && !fs::exists(journal / "committed")) {
            const auto old = journal / "old";
            Require(fs::is_directory(old) && !fs::is_symlink(old));
            // The manifest survives each restore-by-rename. A retry knows which
            // originals were already restored, even if the volume is full.
            const auto manifest = journal / "original";
            const bool fManifest = fs::is_directory(manifest);
            const auto saved = Members(fManifest ? manifest : old, base.filename());
            const auto current = Members(parent, base.filename());
            for (const auto &path : current) {
                if (!saved.contains(path)) {
                    bool fRestored = false;
                    for (const auto &previous : saved) {
                        std::error_code ec;
                        if (fs::equivalent(parent / path, parent / previous, ec) && !ec) {
                            fRestored = true;
                            break;
                        }
                    }
                    if (!fRestored) {
                        fs::remove(parent / path);
                        Sync((parent / path).parent_path());
                    }
                }
            }
            for (const auto &path : saved) {
                const auto source = old / path, target = parent / path;
                if (fManifest && !fs::exists(source)) {
                    Require(fs::is_regular_file(target));
                    Sync(target.parent_path());
                    Sync(source.parent_path());
                } else if (!SameBytes(source, target)) {
                    if (fManifest) {
                        fs::create_directories(target.parent_path());
                        Require(IRenameNativePath(Text(source), Text(target), true, true) == 0);
                        Sync(target.parent_path());
                        Sync(source.parent_path());
                    } else {
                        // Earlier journals retain their only inventory in old/.
                        Copy(source, target);
                        Sync(target.parent_path());
                    }
                }
            }
            for (const auto &entry : fs::directory_iterator(parent)) {
                if (Backup(Fold(Text(entry.path().filename()))) && entry.symlink_status().type() == fs::file_type::directory &&
                    !fs::exists(old / entry.path().filename()) && fs::is_empty(entry.path()))
                    fs::remove(entry.path());
            }
            Sync(parent);
            Require(IRenameNativePath(Text(journal / "ready"), Text(journal / "committed"), true, true) == 0);
            Sync(journal);
        }
        Cleanup();
    }
    void Begin() {
        Require(fs::create_directory(journal));
        Require(fs::create_directory(journal / "old"));
        Require(fs::create_directory(journal / "original"));
        for (const auto &entry : fs::directory_iterator(parent)) {
            if (Backup(Fold(Text(entry.path().filename()))) && entry.symlink_status().type() == fs::file_type::directory)
                fs::create_directory(journal / "old" / entry.path().filename());
        }
        for (const auto &path : Members(parent, base.filename())) {
            const auto copy = journal / "old" / path;
            std::error_code ec;
            // Rotation only replaces or unlinks backup entries. A hard link
            // preserves their original bytes without duplicating the archive.
            if (path.has_parent_path()) fs::create_hard_link(parent / path, copy, ec);
            if (!path.has_parent_path() || ec) Copy(parent / path, copy);
            Sync(copy.parent_path());
            const auto receipt = journal / "original" / path;
            fs::create_directories(receipt.parent_path());
            NATIVEFILEOWNER file(PfNativeOpen(Text(receipt), NATIVEFILEMODE::writeExclusive), ICloseNativeFile);
            Require(bool(file));
            Sync(receipt.parent_path());
        }
        Sync(journal / "original");
        Sync(journal / "old");
        Sync(journal);
        Sync(journal.parent_path());
        Sync(journal.parent_path().parent_path());
        Sync(parent);
        Marker(journal / "ready");
    }
    bool Finish() {
        for (const auto &path : Members(parent, base.filename())) {
            const auto old = journal / "old" / path, live = parent / path;
            if (fs::exists(old) && fs::equivalent(old, live)) continue;
            NATIVEFILEOWNER file(PfNativeOpenFlags(Text(live), {.mdAccess = NATIVEFILEACCESS::readWrite, .fNative = true}), ICloseNativeFile);
            if (file) Require(ISyncNativeFile(file.get()) == 0);
            else Require(fs::exists(old) && SameBytes(old, live));
            Sync((parent / path).parent_path());
        }
        Sync(parent);
        Require(IRenameNativePath(Text(journal / "ready"), Text(journal / "committed"), true, true) == 0);
        if (ISyncNativeDirectory(Text(journal)) != 0) {
            Require(IRenameNativePath(Text(journal / "committed"), Text(journal / "ready"), true, true) == 0);
            return false;
        }
        // Cleanup is retryable; the durable commit marker is authoritative.
        try {
            Cleanup();
        } catch (...) {
        }
        return true;
    }
};
NATIVEFAMILY::NATIVEFAMILY(std::string_view szBase) noexcept {
    try {
        const auto path = PathFromUtf8(szBase);
        const auto stem = PathFromUtf8(Fold(Text(path.filename())));
        // The legacy writer strips a suffix on every open. A remaining dot
        // would redirect a later write outside the locked family.
        Require(!stem.empty() && Text(stem).find('.') == std::string::npos);
        auto state = std::make_unique<NATIVEFAMILYSTATE>();
        state->parent = fs::canonical(path.has_parent_path() ? path.parent_path() : fs::path("."));
#ifdef _WIN32
        // Journal names include the family stem twice. Use native extended paths
        // even when the corresponding game files fit in the legacy path limit.
        const auto szParent = state->parent.make_preferred().native();
        if (!szParent.starts_with(LR"(\\?\)")) {
            state->parent = szParent.starts_with(LR"(\\)")
                                ? LR"(\\?\UNC\)" + szParent.substr(2)
                                : LR"(\\?\)" + szParent;
        }
#endif
        state->base = state->parent / stem;
        const auto control = state->parent / "save-transactions" / stem;
        Require(!fs::is_symlink(control.parent_path()) && !fs::is_symlink(control));
        fs::create_directories(control);
        Require(!fs::is_symlink(control / "lock"));
        state->lock = NfoNativeLock(Text(control / "lock"));
        Require(bool(state->lock));
        state->journal = control / "transaction";
        state->Recover();
        try {
            state->Begin();
        } catch (...) {
            state->Recover();
            throw;
        }
        lpState = std::move(state);
    } catch (...) {
        // A failed recovery leaves the journal in place for the next writer.
    }
}

NATIVEFAMILY::~NATIVEFAMILY() { Rollback(); }

bool NATIVEFAMILY::Rollback() noexcept {
    if (!lpState) return true;
    try {
        lpState->Recover();
        return true;
    } catch (...) {
        return false;
    }
}

bool NATIVEFAMILY::Commit() noexcept {
    if (!lpState) return false;
    try {
        if (!lpState->Finish()) return false;
        lpState.reset();
        return true;
    } catch (...) {
        return false;
    }
}

bool FCopyNativeFileDurable(std::string_view szSource, std::string_view szTarget) noexcept {
    try {
        Copy(PathFromUtf8(szSource), PathFromUtf8(szTarget));
        return true;
    } catch (...) {
        return false;
    }
}
