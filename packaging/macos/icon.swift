#!/usr/bin/env swift
// Package the approved artwork with room for macOS's Dock/switcher highlight.
import AppKit

let urlRoot = URL(fileURLWithPath: CommandLine.arguments.count > 1 ? CommandLine.arguments[1] : ".")
let urlIcons = urlRoot.appendingPathComponent("res/icons")
let lpImage = NSImage(contentsOf: urlIcons.appendingPathComponent("stars-macos-v3.png"))!
var rgbEntries = Data()

func rgbLength(_ cBytes: Int) -> Data {
    var dwLength = UInt32(cBytes).bigEndian
    return withUnsafeBytes(of: &dwLength) { Data($0) }
}

// ICNS stores PNG chunks for each logical size and Retina representation.
for (cPixels, szType) in [(16, "icp4"), (32, "icp5"), (64, "icp6"), (128, "ic07"),
    (256, "ic08"), (512, "ic09"), (1024, "ic10"), (32, "ic11"), (64, "ic12"),
    (256, "ic13"), (512, "ic14")] {
    let lpBitmap = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: cPixels, pixelsHigh: cPixels,
        bitsPerSample: 8, samplesPerPixel: 4, hasAlpha: true, isPlanar: false,
        colorSpaceName: .deviceRGB, bytesPerRow: 0, bitsPerPixel: 0)!
    NSGraphicsContext.saveGraphicsState()
    NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: lpBitmap)
    NSColor.clear.setFill()
    NSRect(x: 0, y: 0, width: cPixels, height: cPixels).fill(using: .copy)
    NSGraphicsContext.current!.imageInterpolation = .high
    // Inset the source canvas by 1/16 on each side, keeping its own transparency.
    let dInset = CGFloat(cPixels) / 16
    lpImage.draw(in: NSRect(x: dInset, y: dInset, width: CGFloat(cPixels) - 2 * dInset,
        height: CGFloat(cPixels) - 2 * dInset), from: .zero, operation: .copy, fraction: 1)
    NSGraphicsContext.restoreGraphicsState()
    let rgbPng = lpBitmap.representation(using: .png, properties: [:])!
    rgbEntries.append(Data(szType.utf8))
    rgbEntries.append(rgbLength(rgbPng.count + 8))
    rgbEntries.append(rgbPng)
    if szType == "ic09" {
        try rgbPng.write(to: urlIcons.appendingPathComponent("stars-app-macos.png"), options: .atomic)
    }
}
var rgbIcns = Data("icns".utf8)
rgbIcns.append(rgbLength(rgbEntries.count + 8))
rgbIcns.append(rgbEntries)
try rgbIcns.write(to: urlIcons.appendingPathComponent("stars-app.icns"), options: .atomic)
