/* Compile only on macOS. The C++ adapter preserves the narrow Win32 boundary. */
#import <AppKit/AppKit.h>
#include "filedialog_cocoa_bridge.h"
#include "../../../src/platform/errors.h"
#include <algorithm>
#include <climits>
#include <string_view>

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

win32::files::FILESELECTIONRESULT IChooseCocoaFile(
    void *lpWindow, const win32::files::FILESELECTIONREQUEST &request,
    const win32::files::FILESELECTIONPUMP &pfnPump) {
    @autoreleasepool {
        NSSavePanel *panel = request.mode == win32::files::FILESELECTIONMODE::save ? [NSSavePanel savePanel] : [NSOpenPanel openPanel];
        std::string_view szPathView;
        NSString *title = [NSString stringWithUTF8String:request.szTitle.c_str()];
        NSString *folder = [NSString stringWithUTF8String:request.szInitialDirectory.c_str()];
        NSString *name = [NSString stringWithUTF8String:request.szInitialPath.c_str()];
        if (!panel || !title || !folder || !name) return win32::files::FILESELECTIONFAILURE{win32::files::FILESELECTIONERROR::backendFailure};
        panel.title = title; panel.directoryURL = [NSURL fileURLWithPath:folder isDirectory:YES];
        panel.nameFieldStringValue = name; panel.canCreateDirectories = YES;
        if (request.mode != win32::files::FILESELECTIONMODE::save) {
            NSOpenPanel *open = static_cast<NSOpenPanel *>(panel);
            open.canChooseFiles = YES; open.canChooseDirectories = NO; open.allowsMultipleSelection = NO;
        }
        StarsFilePanelDelegate *delegate = [StarsFilePanelDelegate new];
        NSMutableArray<NSString *> *patterns = [NSMutableArray array];
        if (!request.rgFilters.empty()) {
            NSPopUpButton *popup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(0, 0, 300, 26) pullsDown:NO];
            for (const win32::files::FILESELECTIONFILTER &filter : request.rgFilters) {
                NSString *label = [NSString stringWithUTF8String:filter.szName.c_str()];
                NSString *pattern = [NSString stringWithUTF8String:filter.szPattern.c_str()];
                if (!label || !pattern) return win32::files::FILESELECTIONFAILURE{win32::files::FILESELECTIONERROR::backendFailure};
                [popup addItemWithTitle:label]; [patterns addObject:pattern];
            }
            delegate.rgPatterns = patterns;
            delegate.iFilter = request.iFilter < request.rgFilters.size() ? static_cast<NSInteger>(request.iFilter) : 0;
            delegate.panel = panel;
            popup.target = delegate; popup.action = @selector(changeFilter:);
            [popup selectItemAtIndex:delegate.iFilter]; panel.accessoryView = popup; panel.delegate = delegate;
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
                bool fPump = false;
                try { fPump = pfnPump(); }
                catch (...) { StarsCapturePendingError(); }
                if (!fPump) { fCanceled = YES; [panel cancel:nil]; }
            }
            NSEvent *event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]
                                                    inMode:NSDefaultRunLoopMode dequeue:YES];
            if (event) [NSApp sendEvent:event];
            [NSApp updateWindows];
        }
        if (fCanceled || result != NSModalResponseOK) return win32::files::FILESELECTIONCANCELLED{};
        const char *pszSelectedPath = panel.URL.fileSystemRepresentation;
        if (!pszSelectedPath) return win32::files::FILESELECTIONFAILURE{win32::files::FILESELECTIONERROR::backendFailure};
        szPathView = pszSelectedPath;
        if (szPathView.size() >= PATH_MAX) return win32::files::FILESELECTIONFAILURE{win32::files::FILESELECTIONERROR::backendFailure};
        win32::files::FILESELECTIONSELECTED selected;
        selected.szPath = szPathView;
        if (!request.rgFilters.empty()) selected.iFilter = static_cast<std::size_t>(delegate.iFilter);
        return selected;
    }
}
