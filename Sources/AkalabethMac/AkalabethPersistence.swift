import Foundation

public enum AkalabethColorTreatment: String, CaseIterable, Sendable {
    case green
    case amber
}

public struct AkalabethSettings: Equatable, Sendable {
    public var colorTreatment: AkalabethColorTreatment
    public var windowScale: Int

    public init(colorTreatment: AkalabethColorTreatment = .green, windowScale: Int = 2) {
        self.colorTreatment = colorTreatment
        self.windowScale = max(1, min(windowScale, 3))
    }
}

public final class AkalabethPersistence {
    private static let saveHeader = Data("AKALABETHSAVE1\n".utf8)
    private let fileManager: FileManager
    private let defaults: UserDefaults

    public let applicationSupportDirectory: URL
    public let saveURL: URL

    public init(
        applicationSupportDirectory: URL? = nil,
        defaults: UserDefaults = .standard,
        fileManager: FileManager = .default
    ) {
        self.fileManager = fileManager
        self.defaults = defaults
        if let applicationSupportDirectory {
            self.applicationSupportDirectory = applicationSupportDirectory
        } else {
            let base = fileManager.urls(for: .applicationSupportDirectory, in: .userDomainMask).first
                ?? URL(fileURLWithPath: NSHomeDirectory()).appendingPathComponent("Library/Application Support")
            self.applicationSupportDirectory = base.appendingPathComponent("Akalabeth", isDirectory: true)
        }
        self.saveURL = self.applicationSupportDirectory.appendingPathComponent("SaveGame.bin")
    }

    public func loadSettings() -> AkalabethSettings {
        let colorRaw = defaults.string(forKey: Keys.colorTreatment)
        let color = colorRaw.flatMap(AkalabethColorTreatment.init(rawValue:)) ?? .green
        let scale = defaults.object(forKey: Keys.windowScale) == nil ? 2 : defaults.integer(forKey: Keys.windowScale)
        return AkalabethSettings(colorTreatment: color, windowScale: scale)
    }

    public func saveSettings(_ settings: AkalabethSettings) {
        defaults.set(settings.colorTreatment.rawValue, forKey: Keys.colorTreatment)
        defaults.set(settings.windowScale, forKey: Keys.windowScale)
    }

    public func saveSession(_ session: GameSession) throws {
        try ensureApplicationSupportDirectory()
        var data = Self.saveHeader
        data.append(session.snapshot())
        try data.write(to: saveURL, options: [.atomic])
    }

    public func resumeSession() throws -> GameSession? {
        guard fileManager.fileExists(atPath: saveURL.path) else {
            return nil
        }
        let data = try Data(contentsOf: saveURL)
        guard data.starts(with: Self.saveHeader) else {
            return nil
        }
        let snapshot = data.dropFirst(Self.saveHeader.count)
        return GameSession(snapshot: Data(snapshot))
    }

    public func deleteSave() throws {
        guard fileManager.fileExists(atPath: saveURL.path) else {
            return
        }
        try fileManager.removeItem(at: saveURL)
    }

    private func ensureApplicationSupportDirectory() throws {
        try fileManager.createDirectory(at: applicationSupportDirectory, withIntermediateDirectories: true)
    }

    private enum Keys {
        static let colorTreatment = "colorTreatment"
        static let windowScale = "windowScale"
    }
}
