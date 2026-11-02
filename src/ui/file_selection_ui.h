#ifndef STARS_FILE_SELECTION_UI_H
#define STARS_FILE_SELECTION_UI_H

#include "../platform/errors.h"
#include "../../compat/win32/include/file_selection.h"
#include <string_view>
#include <utility>

namespace ui::files {

inline std::vector<win32::files::FILESELECTIONFILTER> RgParseFilters(std::string_view text) {
    std::vector<win32::files::FILESELECTIONFILTER> filters;
    std::size_t i = 0, iNameEnd, iPatternEnd;
    while (i < text.size()) {
        iNameEnd = text.find('|', i);
        if (iNameEnd == std::string_view::npos) break;
        iPatternEnd = text.find('|', iNameEnd + 1);
        if (iPatternEnd == std::string_view::npos) iPatternEnd = text.size();
        if (iNameEnd != i && iPatternEnd != iNameEnd + 1) {
            filters.push_back({std::string{text.substr(i, iNameEnd - i)},
                               std::string{text.substr(iNameEnd + 1, iPatternEnd - iNameEnd - 1)}});
        }
        if (iPatternEnd == text.size()) break;
        i = iPatternEnd + 1;
    }
    return filters;
}

inline win32::files::FILESELECTIONRESULT ChooseFile(HWND hwnd, win32::files::FILESELECTIONREQUEST request) {
    auto result = win32::files::SelectFile(hwnd, std::move(request));
    StarsRethrowPendingError();
    return result;
}

} // namespace ui::files

#endif
