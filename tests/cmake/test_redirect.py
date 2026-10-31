"""Build and run the test adapter contract with independent C and native C++ callers."""
import pathlib
import subprocess
import tempfile
import sys

root = pathlib.Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="stars-test-redirect-") as directory:
    for language in ("C", "CXX"):
        source = pathlib.Path(directory) / language
        source.mkdir()
        suffix = ".c" if language == "C" else ".cpp"
        (source / ("core" + suffix)).write_text('''#include "test_impl.h"
    #ifndef LATE_SETTINGS
    #error private libraries lost the completed target settings
    #endif
    int AlertSz(int x);
    int STARS_TEST_IMPL(AlertSz)(int x) { return x + 1; }
    int CallAlert(void) { return AlertSz(4); }
    int STARS_TEST_IMPL(Random)(int x) { return x + 2; }
    ''')
        (source / ("main" + suffix)).write_text('''int CallAlert(void);
    int Random(int x);
    #ifdef TEST_ALERT
    int StarsTestReal_AlertSz(int x);
    int StarsTest_AlertSz(int x) { return StarsTestReal_AlertSz(x) * 2; }
    #endif
    int main(void) {
        return CallAlert() != EXPECT_ALERT || Random(4) != 6;
    }
    ''')
        # A separate translation unit exercises calls into the second substituted
        # implementation. Direct calls from the harness still use its public API.
        (source / ("random" + suffix)).write_text('''int Random(int x);
    int CallRandom(void) { return Random(4); }
    ''')
        (source / ("both" + suffix)).write_text('''int CallAlert(void), CallRandom(void);
    int AlertSz(int x), Random(int x);
    int StarsTestReal_AlertSz(int x), StarsTestReal_Random(int x);
    int StarsTest_AlertSz(int x) { return StarsTestReal_AlertSz(x) * 2; }
    int StarsTest_Random(int x) { return StarsTestReal_Random(x) * 3; }
    int main(void) { return CallAlert() != 10 || CallRandom() != 18 ||
                           AlertSz(4) != 10 || Random(4) != 18; }
    ''')
        # The harness's direct calls to unredirected public symbols need the
        # original definitions. Restrict the first adapter contract to AlertSz.
        (source / "CMakeLists.txt").write_text(f'''cmake_minimum_required(VERSION 3.23)
    project(redirect_contract C CXX)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_C_STANDARD 11)
    enable_testing()
    include("{root.as_posix()}/cmake/test-redirect.cmake")
    add_library(stars_core OBJECT core{suffix} random{suffix})
    target_include_directories(stars_core PRIVATE "{root.as_posix()}/src/platform")
    add_executable(product main{suffix})
    target_compile_definitions(product PRIVATE EXPECT_ALERT=5)
    target_link_libraries(product PRIVATE stars_core)
    add_executable(mocked main{suffix})
    target_compile_definitions(mocked PRIVATE TEST_ALERT=1 EXPECT_ALERT=10)
    target_link_libraries(mocked PRIVATE stars_core)
    stars_test_redirect(mocked AlertSz)
    add_executable(both both{suffix})
    target_link_libraries(both PRIVATE stars_core)
    stars_test_redirect(both AlertSz)
    stars_test_redirect(both Random)
    target_compile_definitions(stars_core PRIVATE LATE_SETTINGS=1)
    foreach(target product mocked both)
        add_test(NAME ${{target}} COMMAND ${{target}})
    endforeach()
    ''')
        build = source / "build"
        configure = ["cmake", "-S", str(source), "-B", str(build)] + sys.argv[1:]
        subprocess.run(configure, check=True)
        subprocess.run(["cmake", "--build", str(build), "--config", "Debug"], check=True)
        subprocess.run(["ctest", "--test-dir", str(build), "-C", "Debug", "--output-on-failure"], check=True)
