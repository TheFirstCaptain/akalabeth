import Foundation
import CAkalabeth

public enum AkalabethColorTreatment: String, CaseIterable, Sendable {
    case green
    case amber
}

public struct AkalabethSettings: Equatable, Sendable {
    public var colorTreatment: AkalabethColorTreatment
    public var windowScale: Int
    public var integerScaling: Bool
    public var highContrast: Bool
    public var scanlines: Bool
    public var audioEnabled: Bool

    public init(
        colorTreatment: AkalabethColorTreatment = .green,
        windowScale: Int = 2,
        integerScaling: Bool = true,
        highContrast: Bool = false,
        scanlines: Bool = false,
        audioEnabled: Bool = false
    ) {
        self.colorTreatment = colorTreatment
        self.windowScale = max(1, min(windowScale, 3))
        self.integerScaling = integerScaling
        self.highContrast = highContrast
        self.scanlines = scanlines
        self.audioEnabled = audioEnabled
    }
}

public final class AkalabethPersistence {
    private static let saveFormatIdentifier = "com.aklabeth.save"
    private static let saveFormatVersion = 1
    private let fileManager: FileManager
    private let defaults: UserDefaults
    private let appVersion: String

    public let applicationSupportDirectory: URL
    public let saveURL: URL

    public init(
        applicationSupportDirectory: URL? = nil,
        defaults: UserDefaults = .standard,
        fileManager: FileManager = .default,
        appVersion: String = Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "development"
    ) {
        self.fileManager = fileManager
        self.defaults = defaults
        self.appVersion = appVersion
        if let applicationSupportDirectory {
            self.applicationSupportDirectory = applicationSupportDirectory
        } else {
            let base = fileManager.urls(for: .applicationSupportDirectory, in: .userDomainMask).first
                ?? URL(fileURLWithPath: NSHomeDirectory()).appendingPathComponent("Library/Application Support")
            self.applicationSupportDirectory = base.appendingPathComponent("Akalabeth", isDirectory: true)
        }
        self.saveURL = self.applicationSupportDirectory.appendingPathComponent("SaveGame.json")
    }

    public func loadSettings() -> AkalabethSettings {
        let colorRaw = defaults.string(forKey: Keys.colorTreatment)
        let color = colorRaw.flatMap(AkalabethColorTreatment.init(rawValue:)) ?? .green
        let scale = defaults.object(forKey: Keys.windowScale) == nil ? 2 : defaults.integer(forKey: Keys.windowScale)
        let integerScaling = defaults.object(forKey: Keys.integerScaling) == nil ? true : defaults.bool(forKey: Keys.integerScaling)
        let highContrast = defaults.bool(forKey: Keys.highContrast)
        let scanlines = defaults.bool(forKey: Keys.scanlines)
        let audioEnabled = defaults.bool(forKey: Keys.audioEnabled)
        return AkalabethSettings(
            colorTreatment: color,
            windowScale: scale,
            integerScaling: integerScaling,
            highContrast: highContrast,
            scanlines: scanlines,
            audioEnabled: audioEnabled
        )
    }

    public func saveSettings(_ settings: AkalabethSettings) {
        defaults.set(settings.colorTreatment.rawValue, forKey: Keys.colorTreatment)
        defaults.set(settings.windowScale, forKey: Keys.windowScale)
        defaults.set(settings.integerScaling, forKey: Keys.integerScaling)
        defaults.set(settings.highContrast, forKey: Keys.highContrast)
        defaults.set(settings.scanlines, forKey: Keys.scanlines)
        defaults.set(settings.audioEnabled, forKey: Keys.audioEnabled)
    }

    public func saveSession(_ session: GameSession) throws {
        try ensureApplicationSupportDirectory()
        let file = AkalabethSaveFile(
            format: Self.saveFormatIdentifier,
            version: Self.saveFormatVersion,
            savedAt: Date(),
            appVersion: appVersion,
            gameMode: session.state.mode.rawValue,
            displayName: Self.displayName(for: session),
            session: session.savedSession()
        )
        let encoder = JSONEncoder()
        encoder.dateEncodingStrategy = .iso8601
        encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        let data = try encoder.encode(file)
        try data.write(to: saveURL, options: Data.WritingOptions.atomic)
    }

    public func resumeSession() throws -> GameSession? {
        guard fileManager.fileExists(atPath: saveURL.path) else {
            return nil
        }
        let data = try Data(contentsOf: saveURL)
        let decoder = JSONDecoder()
        decoder.dateDecodingStrategy = .iso8601
        guard let file = try? decoder.decode(AkalabethSaveFile.self, from: data),
              file.format == Self.saveFormatIdentifier,
              file.version == Self.saveFormatVersion
        else {
            return nil
        }
        return GameSession(savedSession: file.session)
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

    private static func displayName(for session: GameSession) -> String {
        switch session.state.mode {
        case AK_GAME_MODE_STARTUP_LUCKY_NUMBER, AK_GAME_MODE_STARTUP_LEVEL:
            return "Startup"
        case AK_GAME_MODE_CHARACTER_REVIEW, AK_GAME_MODE_CLASS_SELECT:
            return "Character Creation"
        case AK_GAME_MODE_OVERWORLD:
            return "Overworld"
        case AK_GAME_MODE_TOWN:
            return "Town"
        case AK_GAME_MODE_DUNGEON, AK_GAME_MODE_COMBAT:
            return "Dungeon Level \(session.state.dungeon_level)"
        case AK_GAME_MODE_QUEST:
            return "Lord British"
        case AK_GAME_MODE_DEATH:
            return "Death"
        case AK_GAME_MODE_VICTORY:
            return "Victory"
        default:
            return "Akalabeth Save"
        }
    }

    private struct AkalabethSaveFile: Codable {
        var format: String
        var version: Int
        var savedAt: Date
        var appVersion: String
        var gameMode: UInt32
        var displayName: String
        var session: AkalabethSavedSession
    }

    private enum Keys {
        static let colorTreatment = "colorTreatment"
        static let windowScale = "windowScale"
        static let integerScaling = "integerScaling"
        static let highContrast = "highContrast"
        static let scanlines = "scanlines"
        static let audioEnabled = "audioEnabled"
    }
}
