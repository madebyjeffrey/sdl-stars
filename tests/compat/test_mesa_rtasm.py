"""Test an extracted Mesa source allocator with fatal sanitizers, without a GPU.

Pass the unpatched archive source as a negative control: it leaks 112 bytes per
unload. The tiny headers adapt Mesa utility macros to libc/pthreads; allocator
and executable-heap implementations are compiled unchanged from the source.
"""
import argparse
import pathlib
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=pathlib.Path)
    parser.add_argument('--cc', default='cc')
    args = parser.parse_args()
    source = args.source.resolve()
    with tempfile.TemporaryDirectory(prefix='stars-rtasm-') as directory:
        root = pathlib.Path(directory)
        (root / 'util').mkdir()
        headers = {
            'compiler.h': '#include <stddef.h>\n#define DETECT_OS_POSIX 1\n#define DETECT_OS_WINDOWS 0\n',
            'simple_mtx.h': '#include <pthread.h>\ntypedef pthread_mutex_t simple_mtx_t;\n'
                            '#define SIMPLE_MTX_INITIALIZER PTHREAD_MUTEX_INITIALIZER\n'
                            '#define simple_mtx_lock pthread_mutex_lock\n#define simple_mtx_unlock pthread_mutex_unlock\n',
            'u_debug.h': '#include <stdio.h>\n#define debug_printf(...) fprintf(stderr, __VA_ARGS__)\n',
            'u_memory.h': '#include <stdlib.h>\n#define CALLOC_STRUCT(t) ((struct t *)calloc(1, sizeof(struct t)))\n#define FREE free\n',
            'macros.h': '#include <assert.h>\n#define UNUSED __attribute__((unused))\n',
            'u_thread.h': '',
        }
        for name, content in headers.items():
            (root / 'util' / name).write_text(content)
        shutil.copyfile(source / 'src/util/u_mm.h', root / 'util/u_mm.h')
        rtasm = source / 'src/gallium/auxiliary/rtasm'
        # Copy beside the test headers so include lookup cannot pick up Mesa's
        # full configuration headers instead of these standalone adapters.
        shutil.copyfile(rtasm / 'rtasm_execmem.h', root / 'rtasm_execmem.h')
        shutil.copyfile(rtasm / 'rtasm_execmem.c', root / 'rtasm_execmem.c')
        library = root / 'rtasm.so'
        flags = ['-std=gnu11', '-Wall', '-Wextra', '-Werror', '-g', '-pthread', '-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        subprocess.run([args.cc, *flags, '-shared', '-fPIC', '-I' + str(root),
                        str(source / 'src/util/u_mm.c'), str(root / 'rtasm_execmem.c'),
                        '-o', str(library)], check=True)
        executable = root / 'lifetime'
        subprocess.run([args.cc, *flags, str(pathlib.Path(__file__).with_name('mesa_rtasm_lifetime.c')),
                        '-ldl', '-o', str(executable)], check=True)
        return subprocess.run([str(executable), str(library)]).returncode


if __name__ == '__main__':
    raise SystemExit(main())
