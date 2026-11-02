#ifndef STARS_FILE_SELECTION_H
#define STARS_FILE_SELECTION_H

#include "wincompat_types.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace win32::files {

enum class FILESELECTIONMODE : std::uint8_t { open, save };

struct FILESELECTIONFILTER {
    std::string szName;
    std::string szPattern;
};

enum class FILESELECTIONERROR : std::uint8_t {
    invalidRequest,
    invalidPath,
    unsupportedOption,
    ownerUnavailable,
    allocationFailure,
    bufferTooSmall,
    backendFailure,
};

struct FILESELECTIONREQUEST {
    FILESELECTIONMODE mode = FILESELECTIONMODE::open;
    std::string szTitle;
    std::string szInitialDirectory;
    std::string szInitialPath;
    std::string szDefaultExtension;
    std::vector<FILESELECTIONFILTER> rgFilters;
    std::size_t iFilter = 0;
    // When set, capacity includes the terminating NUL.
    std::optional<std::size_t> cbPathMax;
    bool fMustExist = false;
    bool fRequireWritable = false;
    bool fConfirmOverwrite = false;
    bool fChangeCurrentDirectory = true;
};

struct FILESELECTIONCANCELLED {};

struct FILESELECTIONSELECTED {
    std::string szPath;
    std::optional<std::size_t> iFilter;
    bool fExtensionDifferent = false;
};

struct FILESELECTIONFAILURE {
    FILESELECTIONERROR error;
    // Required path bytes include the terminating NUL for bufferTooSmall.
    std::size_t cbRequiredPath = 0;
};

using FILESELECTIONRESULT = std::variant<FILESELECTIONSELECTED, FILESELECTIONCANCELLED, FILESELECTIONFAILURE>;

// Requests own their text through the modal pump. hwndOwner is borrowed only
// for this call; implementations recheck it after every modal callback.
FILESELECTIONRESULT SelectFile(HWND hwndOwner, FILESELECTIONREQUEST request);

} // namespace win32::files

#endif
