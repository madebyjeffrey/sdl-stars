/* Compile only on macOS. The C++ adapter preserves the narrow Win32 boundary. */
#import <AppKit/AppKit.h>
#include "filedialog_cocoa_bridge.h"
#include "../../../src/platform/errors.h"
#include <string.h>

@interface StarsFilePanelDelegate : NSObject <NSOpenSavePanelDelegate>
@property(nonatomic, strong) NSArray<NSString *> *rgPatterns;
@property(nonatomic) NSInteger iFilter;
@property(nonatomic, weak) NSSavePanel *panel;
@end
@implementation StarsFilePanelDelegate
- (BOOL)panel:(id)sender shouldEnableURL:(NSURL *)url {
    NSNumber *fDirectory = nil;
    static_cast<void>(sender);
    [url getResourceValue:&fDirectory forKey:NSURLIsDirectoryKey error:nil];
    if (fDirectory.boolValue || self.rgPatterns.count == 0) return YES;
    for (NSString *pattern in [self.rgPatterns[self.iFilter] componentsSeparatedByString:@";"]) {
        if ([pattern isEqualToString:@"*.*"]) return YES;
        NSPredicate *predicate = [NSPredicate predicateWithFormat:@"SELF LIKE[c] %@", pattern];
        if ([predicate evaluateWithObject:url.lastPathComponent]) return YES;
    }
    return NO;
}
- (void)changeFilter:(NSPopUpButton *)sender {
    self.iFilter = sender.indexOfSelectedItem;
    [self.panel validateVisibleColumns];
}
@end

int IChooseCocoaFile(void *lpWindow, int fSave, const char *pszTitle, const char *pszFolder,
                     const char *pszName, const COCOAFILEFILTER *rgfilter, int cFilter,
                     int *lpiFilter, char *pszPath, size_t cbPath,
                     int (*pfnPump)(void *), void *lpv) {
    @autoreleasepool {
        NSSavePanel *panel = fSave ? [NSSavePanel savePanel] : [NSOpenPanel openPanel];
        NSString *title = [NSString stringWithUTF8String:pszTitle];
        NSString *folder = [NSString stringWithUTF8String:pszFolder];
        NSString *name = [NSString stringWithUTF8String:pszName];
        if (!panel || !title || !folder || !name) return -1;
        panel.title = title; panel.directoryURL = [NSURL fileURLWithPath:folder isDirectory:YES];
        panel.nameFieldStringValue = name; panel.canCreateDirectories = YES;
        if (!fSave) {
            NSOpenPanel *open = static_cast<NSOpenPanel *>(panel);
            open.canChooseFiles = YES; open.canChooseDirectories = NO; open.allowsMultipleSelection = NO;
        }
        StarsFilePanelDelegate *delegate = [StarsFilePanelDelegate new];
        NSMutableArray<NSString *> *patterns = [NSMutableArray array];
        if (cFilter != 0) {
            NSPopUpButton *popup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(0, 0, 300, 26) pullsDown:NO];
            for (int i = 0; i < cFilter; i++) {
                NSString *label = [NSString stringWithUTF8String:rgfilter[i].pszName];
                NSString *pattern = [NSString stringWithUTF8String:rgfilter[i].pszPattern];
                if (!label || !pattern) return -1;
                [popup addItemWithTitle:label]; [patterns addObject:pattern];
            }
            delegate.rgPatterns = patterns; delegate.iFilter = *lpiFilter; delegate.panel = panel;
            popup.target = delegate; popup.action = @selector(changeFilter:);
            [popup selectItemAtIndex:*lpiFilter]; panel.accessoryView = popup; panel.delegate = delegate;
        }
        __block BOOL fDone = NO;
        __block NSInteger result = NSModalResponseCancel;
        void (^completion)(NSModalResponse) = ^(NSModalResponse response) { result = response; fDone = YES; };
        NSWindow *parent = (__bridge NSWindow *)lpWindow;
        if (parent) [panel beginSheetModalForWindow:parent completionHandler:completion];
        else [panel beginWithCompletionHandler:completion];
        BOOL fCanceled = NO;
        while (!fDone) {
            if (!fCanceled) {
                int fPump = 0;
                try { fPump = pfnPump(lpv); }
                catch (...) { StarsCapturePendingError(); }
                if (!fPump) { fCanceled = YES; [panel cancel:nil]; }
            }
            NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]
                                                    inMode:NSDefaultRunLoopMode dequeue:YES];
            if (event) [NSApp sendEvent:event];
            [NSApp updateWindows];
        }
        if (fCanceled || result != NSModalResponseOK) return 0;
        const char *path = panel.URL.fileSystemRepresentation;
        if (!path || strlen(path) >= cbPath) return -1;
        memcpy(pszPath, path, strlen(path) + 1);
        if (cFilter != 0) *lpiFilter = static_cast<int>(delegate.iFilter);
        return 1;
    }
}
