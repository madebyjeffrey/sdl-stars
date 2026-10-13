/* Native-sheet acceptance; called only by the opt-in desktop test. */
#import <AppKit/AppKit.h>
#include <stdio.h>

int FTestCocoaSaveOwner(void *lpWindow) {
    NSWindow *parent = (__bridge NSWindow *)lpWindow;
    NSWindow *sheet = parent.attachedSheet;

    if (!parent.isVisible || !sheet.isVisible || ![sheet isKindOfClass:[NSSavePanel class]]) return 0;
    [(NSSavePanel *)sheet cancel:nil];
    return 1;
}

int FTestCocoaAppIcon(void) {
    NSBitmapImageRep *image = [NSBitmapImageRep imageRepWithData:NSApp.applicationIconImage.TIFFRepresentation];

    fprintf(stderr, "Dock image: %ld x %ld; corner alpha: %.3f\n", (long)image.pixelsWide,
        (long)image.pixelsHigh, [image colorAtX:0 y:0].alphaComponent);

    // TIFF rendering can double the pixel dimensions on a Retina display.
    return NSApp.applicationIconImage.size.width == 512 && NSApp.applicationIconImage.size.height == 512 &&
        image.pixelsWide >= 512 && image.pixelsHigh >= 512 &&
        [image colorAtX:0 y:0].alphaComponent < 0.01;
}
