import AppKit
import AkalabethMac
import CAkalabeth
import Darwin

@MainActor
final class AppDelegate: NSObject, NSApplicationDelegate {
    private let persistence: AkalabethPersistence
    private var settings: AkalabethSettings
    private var window: NSWindow?
    private var gameView: GameView?
    private var session: GameSession
    private var colorMenuItems: [AkalabethColorTreatment: NSMenuItem] = [:]
    private var scaleMenuItems: [Int: NSMenuItem] = [:]

    override init() {
        let persistence = AkalabethPersistence()
        let settings = persistence.loadSettings()
        let fixture = AppDelegate.fixtureFromArguments()
        let restored = fixture == .normal ? try? persistence.resumeSession() : nil

        self.persistence = persistence
        self.settings = settings
        self.session = restored ?? GameSession(fixture: fixture)
        if restored != nil {
            self.session.setStatusLine("Resumed saved game")
        }
        super.init()
    }

    func applicationDidFinishLaunching(_ notification: Notification) {
        buildMenu()
        buildWindow()
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        true
    }

    @objc private func newGame() {
        session.reset()
        try? persistence.deleteSave()
        gameView?.session = session
        saveCurrentSession()
    }

    @objc private func resetGame() {
        session.reset(fixture: AppDelegate.fixtureFromArguments())
        gameView?.session = session
        saveCurrentSession()
    }

    @objc private func saveGame() {
        saveCurrentSession()
        session.setStatusLine("Game saved")
        gameView?.needsDisplay = true
    }

    @objc private func resumeSavedGame() {
        guard let resumed = try? persistence.resumeSession() else {
            session.setStatusLine("No saved game")
            gameView?.needsDisplay = true
            return
        }
        session = resumed
        session.setStatusLine("Resumed saved game")
        gameView?.session = session
    }

    @objc private func deleteSavedGame() {
        try? persistence.deleteSave()
        session.setStatusLine("Saved game deleted")
        gameView?.needsDisplay = true
    }

    @objc private func setGreenColor() {
        updateSettings(colorTreatment: .green)
    }

    @objc private func setAmberColor() {
        updateSettings(colorTreatment: .amber)
    }

    @objc private func setScale1() {
        updateSettings(windowScale: 1)
    }

    @objc private func setScale2() {
        updateSettings(windowScale: 2)
    }

    @objc private func setScale3() {
        updateSettings(windowScale: 3)
    }

    @objc private func launchTownFixture() {
        loadFixture(.town)
    }

    @objc private func launchOverworldFixture() {
        loadFixture(.overworld)
    }

    @objc private func launchDungeonFixture() {
        loadFixture(.dungeon)
    }

    @objc private func launchQuestFixture() {
        loadFixture(.quest)
    }

    private func loadFixture(_ fixture: AkalabethFixture) {
        session = GameSession(fixture: fixture)
        gameView?.session = session
    }

    private func buildWindow() {
        let contentSize = AppDelegate.windowSize(for: settings.windowScale)
        let view = GameView(frame: NSRect(origin: .zero, size: contentSize))
        view.session = session
        view.settings = settings
        view.onSessionChanged = { [weak self] _ in
            self?.saveCurrentSession()
        }
        gameView = view

        let window = NSWindow(
            contentRect: NSRect(origin: .zero, size: contentSize),
            styleMask: [.titled, .closable, .miniaturizable, .resizable],
            backing: .buffered,
            defer: false
        )
        window.title = "Akalabeth"
        window.center()
        window.contentView = view
        window.makeFirstResponder(view)
        window.makeKeyAndOrderFront(nil)
        self.window = window
    }

    private func buildMenu() {
        let mainMenu = NSMenu()
        NSApp.mainMenu = mainMenu

        let appItem = NSMenuItem()
        mainMenu.addItem(appItem)
        let appMenu = NSMenu()
        appItem.submenu = appMenu
        appMenu.addItem(withTitle: "Quit Akalabeth", action: #selector(NSApplication.terminate(_:)), keyEquivalent: "q")

        let gameItem = NSMenuItem()
        mainMenu.addItem(gameItem)
        let gameMenu = NSMenu(title: "Game")
        gameItem.submenu = gameMenu
        gameMenu.addItem(withTitle: "New Game", action: #selector(newGame), keyEquivalent: "n").target = self
        gameMenu.addItem(withTitle: "Reset", action: #selector(resetGame), keyEquivalent: "r").target = self
        gameMenu.addItem(withTitle: "Save Game", action: #selector(saveGame), keyEquivalent: "s").target = self
        gameMenu.addItem(withTitle: "Resume Saved Game", action: #selector(resumeSavedGame), keyEquivalent: "o").target = self
        gameMenu.addItem(withTitle: "Delete Saved Game", action: #selector(deleteSavedGame), keyEquivalent: "").target = self
        gameMenu.addItem(.separator())
        gameMenu.addItem(withTitle: "Debug Town Fixture", action: #selector(launchTownFixture), keyEquivalent: "1").target = self
        gameMenu.addItem(withTitle: "Debug Overworld Fixture", action: #selector(launchOverworldFixture), keyEquivalent: "2").target = self
        gameMenu.addItem(withTitle: "Debug Dungeon Fixture", action: #selector(launchDungeonFixture), keyEquivalent: "3").target = self
        gameMenu.addItem(withTitle: "Debug Quest Fixture", action: #selector(launchQuestFixture), keyEquivalent: "4").target = self

        let settingsItem = NSMenuItem()
        mainMenu.addItem(settingsItem)
        let settingsMenu = NSMenu(title: "Settings")
        settingsItem.submenu = settingsMenu
        let green = settingsMenu.addItem(withTitle: "Green Display", action: #selector(setGreenColor), keyEquivalent: "")
        green.target = self
        colorMenuItems[.green] = green
        let amber = settingsMenu.addItem(withTitle: "Amber Display", action: #selector(setAmberColor), keyEquivalent: "")
        amber.target = self
        colorMenuItems[.amber] = amber
        settingsMenu.addItem(.separator())
        for scale in 1...3 {
            let item = settingsMenu.addItem(withTitle: "Window Scale \(scale)x", action: scaleSelector(scale), keyEquivalent: "")
            item.target = self
            scaleMenuItems[scale] = item
        }
        refreshSettingsMenu()
    }

    private func saveCurrentSession() {
        try? persistence.saveSession(session)
    }

    private func updateSettings(colorTreatment: AkalabethColorTreatment? = nil, windowScale: Int? = nil) {
        if let colorTreatment {
            settings.colorTreatment = colorTreatment
        }
        if let windowScale {
            settings.windowScale = windowScale
            window?.setContentSize(AppDelegate.windowSize(for: windowScale))
            window?.center()
        }
        persistence.saveSettings(settings)
        gameView?.settings = settings
        refreshSettingsMenu()
    }

    private func refreshSettingsMenu() {
        for (color, item) in colorMenuItems {
            item.state = settings.colorTreatment == color ? .on : .off
        }
        for (scale, item) in scaleMenuItems {
            item.state = settings.windowScale == scale ? .on : .off
        }
    }

    private func scaleSelector(_ scale: Int) -> Selector {
        switch scale {
        case 1:
            return #selector(setScale1)
        case 2:
            return #selector(setScale2)
        default:
            return #selector(setScale3)
        }
    }

    private static func windowSize(for scale: Int) -> NSSize {
        let clamped = max(1, min(scale, 3))
        return NSSize(width: 420 * clamped, height: 300 * clamped)
    }

    private static func fixtureFromArguments() -> AkalabethFixture {
        for argument in CommandLine.arguments {
            if argument.hasPrefix("--fixture=") {
                let value = String(argument.dropFirst("--fixture=".count))
                return AkalabethFixture(rawValue: value) ?? .normal
            }
        }
        return .normal
    }

    static func runSmokeTest() {
        for fixture in [AkalabethFixture.normal, .town, .overworld, .dungeon, .quest] {
            let session = GameSession(fixture: fixture)
            var buffer = session.renderCommands()
            precondition(buffer.count > 0, "empty render buffer for \(fixture.rawValue)")
            _ = withUnsafePointer(to: &buffer.commands) { pointer in
                pointer.withMemoryRebound(to: AkRenderCommand.self, capacity: max(Int(buffer.count), 1)) { commands in
                    commands[0].type
                }
            }
        }
    }
}

@main
enum AkalabethLauncher {
    @MainActor
    static func main() {
        if CommandLine.arguments.contains("--smoke-test") {
            AppDelegate.runSmokeTest()
            Darwin.exit(0)
        }

        let application = NSApplication.shared
        let delegate = AppDelegate()
        application.delegate = delegate
        application.setActivationPolicy(.regular)
        application.run()
    }
}

final class GameView: NSView {
    var session: GameSession = GameSession() {
        didSet {
            needsDisplay = true
        }
    }
    var settings = AkalabethSettings() {
        didSet {
            needsDisplay = true
        }
    }
    var onSessionChanged: ((GameSession) -> Void)?

    private let textFont = NSFont.monospacedSystemFont(ofSize: 18, weight: .regular)
    private let smallFont = NSFont.monospacedSystemFont(ofSize: 14, weight: .regular)

    override var acceptsFirstResponder: Bool {
        true
    }

    override func keyDown(with event: NSEvent) {
        if let input = input(from: event) {
            session.handle(input)
            onSessionChanged?(session)
            needsDisplay = true
            return
        }
        super.keyDown(with: event)
    }

    override func draw(_ dirtyRect: NSRect) {
        NSColor.black.setFill()
        bounds.fill()

        var buffer = session.renderCommands()
        let commandsCount = Int(buffer.count)
        let lineScaleX = bounds.width / 280.0
        let lineScaleY = bounds.height / 192.0

        withUnsafePointer(to: &buffer.commands) { pointer in
            pointer.withMemoryRebound(to: AkRenderCommand.self, capacity: max(commandsCount, 1)) { commands in
                for index in 0..<commandsCount {
                    draw(command: commands[index], lineScaleX: lineScaleX, lineScaleY: lineScaleY)
                }
            }
        }

        if !session.inputBuffer.isEmpty || !session.statusLine.isEmpty {
            drawOverlay()
        }
    }

    private func draw(command: AkRenderCommand, lineScaleX: CGFloat, lineScaleY: CGFloat) {
        switch command.type {
        case AK_RENDER_COMMAND_SET_MODE, AK_RENDER_COMMAND_CLEAR:
            return
        case AK_RENDER_COMMAND_SET_COLOR:
            return
        case AK_RENDER_COMMAND_TEXT:
            drawText(commandText(command), column: Int(command.x), row: Int(command.y), inverse: command.inverse != 0)
        case AK_RENDER_COMMAND_PROMPT:
            drawText(commandText(command), column: Int(command.x), row: Int(command.y), inverse: false)
        case AK_RENDER_COMMAND_STATUS:
            drawText("\(commandText(command))=\(command.value)", column: Int(command.x), row: Int(command.y), inverse: false)
        case AK_RENDER_COMMAND_LINE:
            drawLine(command, scaleX: lineScaleX, scaleY: lineScaleY)
        case AK_RENDER_COMMAND_TILE:
            drawTile(command, scaleX: lineScaleX, scaleY: lineScaleY)
        case AK_RENDER_COMMAND_CREATURE:
            drawCreature(command, scaleX: lineScaleX, scaleY: lineScaleY)
        default:
            return
        }
    }

    private func drawLine(_ command: AkRenderCommand, scaleX: CGFloat, scaleY: CGFloat) {
        let path = NSBezierPath()
        path.move(to: point(x: command.x, y: command.y, scaleX: scaleX, scaleY: scaleY))
        path.line(to: point(x: command.x2, y: command.y2, scaleX: scaleX, scaleY: scaleY))
        displayColor.setStroke()
        path.lineWidth = 2
        path.stroke()
    }

    private func drawTile(_ command: AkRenderCommand, scaleX: CGFloat, scaleY: CGFloat) {
        let center = point(x: command.x, y: command.y, scaleX: scaleX, scaleY: scaleY)
        let rect = NSRect(x: center.x - 28, y: center.y - 18, width: 56, height: 36)
        displayColor.setStroke()
        NSBezierPath(rect: rect).stroke()
        drawString(commandText(command).uppercased(), at: NSPoint(x: rect.minX + 5, y: rect.midY - 7), font: smallFont, inverse: false)
    }

    private func drawCreature(_ command: AkRenderCommand, scaleX: CGFloat, scaleY: CGFloat) {
        let center = point(x: command.x, y: command.y, scaleX: scaleX, scaleY: scaleY)
        let rect = NSRect(x: center.x - 46, y: center.y - 28, width: 92, height: 56)
        displayColor.setStroke()
        NSBezierPath(ovalIn: rect).stroke()
        drawString(commandText(command).uppercased(), at: NSPoint(x: rect.minX + 8, y: rect.midY - 7), font: smallFont, inverse: false)
    }

    private func drawText(_ text: String, column: Int, row: Int, inverse: Bool) {
        let x = CGFloat(max(column - 1, 0)) * 10.0 + 16.0
        let y = bounds.height - CGFloat(max(row, 1)) * 22.0 - 8.0
        drawString(text, at: NSPoint(x: x, y: y), font: textFont, inverse: inverse)
    }

    private func drawOverlay() {
        let text = session.inputBuffer.isEmpty ? session.statusLine : session.inputBuffer
        drawString(text, at: NSPoint(x: 16, y: 16), font: textFont, inverse: true)
    }

    private func drawString(_ text: String, at point: NSPoint, font: NSFont, inverse: Bool) {
        let attributes: [NSAttributedString.Key: Any] = [
            .font: font,
            .foregroundColor: inverse ? NSColor.black : displayColor,
            .backgroundColor: inverse ? displayColor : NSColor.clear
        ]
        text.draw(at: point, withAttributes: attributes)
    }

    private var displayColor: NSColor {
        switch settings.colorTreatment {
        case .green:
            return .systemGreen
        case .amber:
            return NSColor(calibratedRed: 1.0, green: 0.68, blue: 0.2, alpha: 1.0)
        }
    }

    private func point(x: Int32, y: Int32, scaleX: CGFloat, scaleY: CGFloat) -> NSPoint {
        NSPoint(x: CGFloat(x) * scaleX, y: bounds.height - CGFloat(y) * scaleY)
    }

    private func commandText(_ command: AkRenderCommand) -> String {
        var mutable = command
        return withUnsafePointer(to: &mutable.text) { pointer in
            pointer.withMemoryRebound(to: CChar.self, capacity: Int(AK_RENDER_TEXT_SIZE)) { chars in
                String(cString: chars)
            }
        }
    }

    private func input(from event: NSEvent) -> AkalabethInput? {
        switch event.keyCode {
        case 36:
            return .enter
        case 53:
            return .escape
        case 123:
            return .left
        case 124:
            return .right
        case 125:
            return .down
        case 126:
            return .up
        default:
            guard let first = event.charactersIgnoringModifiers?.first else {
                return nil
            }
            return .character(first)
        }
    }
}
