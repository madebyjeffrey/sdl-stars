/* Check the native backing before the first popup renderer exists. */
#import <AppKit/AppKit.h>
#include <stdio.h>

int FTestCocoaPopupBackground(void *lpWindow) {
    NSWindow *window = (__bridge NSWindow *)lpWindow;

    if (window.isVisible || window.isOpaque || window.backgroundColor.alphaComponent != 0)
        fprintf(stderr, "Native popup before paint: visible=%d opaque=%d alpha=%.2f\n",
                window.isVisible, window.isOpaque, window.backgroundColor.alphaComponent);
    return !window.isVisible && !window.isOpaque && window.backgroundColor.alphaComponent == 0;
}
