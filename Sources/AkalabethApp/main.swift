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
        var currentMode = AK_RENDER_MODE_TEXT

        withUnsafePointer(to: &buffer.commands) { pointer in
            pointer.withMemoryRebound(to: AkRenderCommand.self, capacity: max(commandsCount, 1)) { commands in
                for index in 0..<commandsCount {
                    draw(command: commands[index], currentMode: &currentMode)
                }
            }
        }

        if !session.inputBuffer.isEmpty || !session.statusLine.isEmpty {
            drawOverlay()
        }
    }

    private func draw(command: AkRenderCommand, currentMode: inout AkRenderMode) {
        switch command.type {
        case AK_RENDER_COMMAND_SET_MODE:
            currentMode = command.mode
        case AK_RENDER_COMMAND_CLEAR:
            return
        case AK_RENDER_COMMAND_SET_COLOR:
            return
        case AK_RENDER_COMMAND_TEXT:
            if currentMode == AK_RENDER_MODE_HIRES && (command.x > 40 || command.y > 24) {
                drawHiresText(commandText(command), x: command.x, y: command.y, inverse: command.inverse != 0)
            } else {
                drawText(commandText(command), column: Int(command.x), row: Int(command.y), inverse: command.inverse != 0)
            }
        case AK_RENDER_COMMAND_PROMPT:
            drawText(commandText(command), column: Int(command.x), row: Int(command.y), inverse: false)
        case AK_RENDER_COMMAND_STATUS:
            drawText("\(commandText(command))=\(command.value)", column: Int(command.x), row: Int(command.y), inverse: false)
        case AK_RENDER_COMMAND_LINE:
            drawLine(command)
        case AK_RENDER_COMMAND_TILE:
            drawTile(command, mode: currentMode)
        case AK_RENDER_COMMAND_CREATURE:
            drawCreature(command)
        default:
            return
        }
    }

    private func drawLine(_ command: AkRenderCommand) {
        let path = NSBezierPath()
        path.move(to: point(x: command.x, y: command.y))
        path.line(to: point(x: command.x2, y: command.y2))
        displayColor.setStroke()
        path.lineWidth = max(1.0, pixelScale)
        path.stroke()
    }

    private func drawTile(_ command: AkRenderCommand, mode: AkRenderMode) {
        let name = commandText(command)
        if mode == AK_RENDER_MODE_HIRES && isOverworldTile(name) {
            drawOverworldTile(command)
            return
        }
        drawDungeonTile(command)
    }

    private func drawCreature(_ command: AkRenderCommand) {
        let monster = Int(command.value)
        let distance = max(1, Int(command.x2))
        let c = CGFloat(command.x)
        let b = monsterBaseY(distance: distance)
        let d = CGFloat(distance)
        let path = NSBezierPath()

        func p(_ dx: CGFloat, _ dy: CGFloat) -> NSPoint {
            point(x: c + dx / d, y: b + dy / d)
        }

        func stroke(_ points: [(CGFloat, CGFloat)]) {
            guard let first = points.first else {
                return
            }
            path.move(to: p(first.0, first.1))
            for point in points.dropFirst() {
                path.line(to: p(point.0, point.1))
            }
        }

        switch monster {
        case 1:
            stroke([(-23, 0), (-15, 0), (-15, -15), (-8, -30), (8, -30), (15, -15), (15, 0), (23, 0)])
            stroke([(0, -26), (0, -65)])
            stroke([(-23, -56), (-30, -53), (-23, -45), (-23, -53), (-8, -38)])
            stroke([(-15, -45), (-8, -60), (8, -60), (15, -45)])
            stroke([(15, -42), (15, -57)])
            stroke([(-5, -72), (5, -72)])
        case 2:
            stroke([(0, -56), (0, -8), (10, 0), (30, 0), (30, -45), (10, -64), (0, -56)])
            stroke([(0, -56), (-10, -64), (-30, -45), (-30, 0), (-10, 0), (0, -8)])
            stroke([(-10, -64), (-10, -75), (0, -83), (10, -75), (0, -79), (-10, -75), (0, -60), (10, -75), (10, -64)])
        case 3:
            stroke([(5, -30), (0, -25), (-5, -30), (-15, -5), (-10, 0), (10, 0), (15, -5), (20, -5), (10, 0), (15, -5), (5, -30), (10, -40)])
            stroke([(10, -40), (3.5, -35), (-3.5, -35), (-10, -40), (-5, -30)])
            stroke([(-5, -33), (-3.5, -30)])
            stroke([(5, -33), (3.5, -30)])
            stroke([(-7, -20), (-7, -15)])
            stroke([(7, -20), (7, -15)])
        case 4:
            stroke([(0, 0), (-15, 0), (-8, -8), (-8, -15), (-15, -23), (-15, -15), (-23, -23), (-23, -45), (-15, -53), (-8, -53), (-15, -68), (-8, -75), (0, -75)])
            stroke([(0, 0), (15, 0), (8, -8), (8, -15), (15, -23), (15, -15), (23, -23), (23, -45), (15, -53), (8, -53), (15, -68), (8, -75), (0, -75)])
            stroke([(-15, -68), (15, -68)])
            stroke([(-8, -53), (8, -53)])
            stroke([(-23, -15), (8, -45)])
            stroke([(0, -38), (-8, -38), (8, -53), (8, -45), (15, -45), (0, -30), (0, -38)])
        case 5:
            stroke([(-10, -15), (-10, -30), (-15, -20), (-15, -15), (-15, 0), (15, 0), (15, -15), (-15, -15)])
            stroke([(-15, -10), (15, -10)])
            stroke([(-15, -5), (15, -5)])
            stroke([(0, -15), (-5, -20), (-5, -35), (5, -35), (5, -20), (10, -15)])
            stroke([(-5, -20), (5, -20)])
            stroke([(-10, -40), (0, -45), (10, -40)])
            stroke([(-15, -30), (0, -40), (15, -30)])
        case 6:
            stroke([(-20, -9), (-20, -88), (-10, -83), (10, -83), (20, -88), (20, -9), (-20, -9)])
            stroke([(-20, -88), (-30, -83), (-30, -78)])
            stroke([(20, -88), (30, -83), (40, -83)])
            stroke([(-15, -86), (-20, -83), (-20, -78), (-30, -73), (-30, -68), (-20, -63)])
            stroke([(-10, -83), (-10, -58), (0, -50)])
            stroke([(10, -83), (10, -78), (20, -73), (20, -40)])
            stroke([(0, -83), (0, -73), (10, -68), (10, -63), (0, -58)])
        case 7:
            stroke([(5.5, -10), (-4.5, -10), (0, -15), (10, -20), (5.5, -15), (5.5, -10), (7.5, -6), (5.5, -3), (-4.5, -3), (-7.5, -6), (-4.5, -10)])
            stroke([(2.5, -3), (5.5, 0), (8, 0)])
            stroke([(-2.5, -3), (-5.5, 0), (-8, 0)])
            stroke([(3.5, -8), (3.5, -8)])
            stroke([(-3.5, -8), (-3.5, -8)])
            stroke([(3.5, -5), (-3.5, -5)])
        case 8:
            stroke([(-10, 0), (-10, -10), (10, -10), (10, 0), (-10, 0)])
            stroke([(-10, -10), (-5, -15), (15, -15), (15, -5), (10, 0)])
            stroke([(10, -10), (15, -15)])
        case 9:
            stroke([(-14, -46), (-12, -37), (-20, -32), (-30, -32), (-22, -24), (-40, -17), (-40, -7), (-38, -5), (-40, -3), (-40, 0), (-36, 0), (-34, -2), (-32, 0), (-28, 0), (-28, -3), (-30, -5), (-28, -7), (-28, -15), (0, -27)])
            stroke([(14, -46), (12, -37), (20, -32), (30, -32), (22, -24), (40, -17), (40, -7), (38, -5), (40, -3), (40, 0), (36, 0), (34, -2), (32, 0), (28, 0), (28, -3), (30, -5), (28, -7), (28, -15), (0, -27)])
            stroke([(6, -48), (38, -41), (40, -42), (18, -56), (12, -56), (10, -57), (8, -56), (-8, -56), (-10, -58), (14, -58), (16, -59), (8, -63), (6, -63), (2.5, -70)])
            stroke([(-2.5, -70), (-6, -63), (-8, -63), (-16, -59), (-14, -58), (-10, -57), (-12, -56), (-18, -56), (-36, -47), (-36, -39), (-28, -41), (-28, -46), (-20, -50), (-18, -50), (-14, -46)])
        case 10:
            stroke([(6, -60), (30, -90), (60, -30), (60, -10), (30, -40), (15, -40)])
            stroke([(-6, -60), (-30, -90), (-60, -30), (-60, -10), (-30, -40), (-15, -40)])
            stroke([(0, -25), (6, -25), (10, -20), (12, -10), (10, -6), (10, 0), (14, 0), (15, -5), (16, 0), (20, 0), (20, -6), (18, -10), (18, -20), (15, -30), (15, -45), (40, -60), (40, -70)])
            stroke([(0, -25), (-6, -25), (-10, -20), (-12, -10), (-10, -6), (-10, 0), (-14, 0), (-15, -5), (-16, 0), (-20, 0), (-20, -6), (-18, -10), (-18, -20), (-15, -30), (-15, -45), (-40, -60), (-40, -70)])
            stroke([(-6, -25), (0, -6), (10, 0), (4.5, -8), (6, -25)])
            stroke([(-40, -64), (-40, -90), (-52, -80), (-52, -40)])
            stroke([(40, -86), (38, -92), (42, -92), (40, -86), (40, -50)])
            stroke([(4.5, -70), (6, -74)])
            stroke([(-4.5, -70), (-6, -74)])
            stroke([(0, -64), (0, -60)])
        default:
            let center = point(x: command.x, y: command.y)
            let rect = rectAround(center: center, width: 38, height: 50)
            path.appendOval(in: rect)
        }

        displayColor.setStroke()
        path.lineWidth = max(1.0, pixelScale)
        path.stroke()
        drawHiresText(commandText(command).uppercased(), x: 108, y: 10, inverse: true)
    }

    private func drawText(_ text: String, column: Int, row: Int, inverse: Bool) {
        let rect = virtualRect
        let cellWidth = rect.width / 40.0
        let cellHeight = rect.height / 24.0
        let x = rect.minX + CGFloat(max(column - 1, 0)) * cellWidth
        let y = rect.maxY - CGFloat(max(row, 1)) * cellHeight
        drawString(text, at: NSPoint(x: x, y: y), font: textFontForCellHeight(cellHeight), inverse: inverse)
    }

    private func drawHiresText(_ text: String, x: Int32, y: Int32, inverse: Bool) {
        let point = point(x: x, y: y)
        drawString(text, at: point, font: hiresFont, inverse: inverse)
    }

    private func drawOverlay() {
        let text = session.inputBuffer.isEmpty ? session.statusLine : session.inputBuffer
        drawString(text, at: NSPoint(x: virtualRect.minX, y: virtualRect.minY + 2), font: hiresFont, inverse: true)
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

    private func point(x: Int32, y: Int32) -> NSPoint {
        NSPoint(x: virtualRect.minX + CGFloat(x) * pixelScale, y: virtualRect.maxY - CGFloat(y) * pixelScale)
    }

    private func point(x: CGFloat, y: CGFloat) -> NSPoint {
        NSPoint(x: virtualRect.minX + x * pixelScale, y: virtualRect.maxY - y * pixelScale)
    }

    private func monsterBaseY(distance: Int) -> CGFloat {
        79.0 + perspectiveYY(distance)
    }

    private func perspectiveYY(_ distance: Int) -> CGFloat {
        if distance <= 0 {
            return 79.0
        }
        let x = min(max(distance, 1), 10) * 2
        let xx = Int((atan(1.0 / Double(x)) / atan(1.0) * 140.0) + 0.5)
        return CGFloat(Int(Double(xx) * 4.0 / 7.0))
    }

    private func rectAround(center: NSPoint, width: CGFloat, height: CGFloat) -> NSRect {
        NSRect(
            x: center.x - width * pixelScale / 2.0,
            y: center.y - height * pixelScale / 2.0,
            width: width * pixelScale,
            height: height * pixelScale
        )
    }

    private var virtualRect: NSRect {
        let scale = min(bounds.width / 280.0, bounds.height / 192.0)
        let size = NSSize(width: 280.0 * scale, height: 192.0 * scale)
        return NSRect(
            x: bounds.midX - size.width / 2.0,
            y: bounds.midY - size.height / 2.0,
            width: size.width,
            height: size.height
        )
    }

    private var pixelScale: CGFloat {
        min(bounds.width / 280.0, bounds.height / 192.0)
    }

    private var hiresFont: NSFont {
        NSFont.monospacedSystemFont(ofSize: max(8.0, 8.0 * pixelScale), weight: .regular)
    }

    private func textFontForCellHeight(_ cellHeight: CGFloat) -> NSFont {
        NSFont.monospacedSystemFont(ofSize: max(7.0, min(18.0 * pixelScale, cellHeight * 0.78)), weight: .regular)
    }

    private func isOverworldTile(_ name: String) -> Bool {
        switch name {
        case "open", "mountain", "field", "town", "dungeon", "castle":
            return true
        default:
            return false
        }
    }

    private func drawOverworldTile(_ command: AkRenderCommand) {
        let center = point(x: command.x, y: command.y)
        let rect = rectAround(center: center, width: 50, height: 50)
        let path = NSBezierPath()
        let s = pixelScale
        let x = rect.minX
        let y = rect.minY

        switch UInt32(command.value) {
        case AK_GAME_TILE_MOUNTAIN.rawValue:
            path.move(to: NSPoint(x: x + 10 * s, y: y))
            path.line(to: NSPoint(x: x + 10 * s, y: y + 10 * s))
            path.line(to: NSPoint(x: x + 20 * s, y: y + 20 * s))
            path.line(to: NSPoint(x: x + 40 * s, y: y + 20 * s))
            path.line(to: NSPoint(x: x + 40 * s, y: y))
            path.move(to: NSPoint(x: x, y: y + 40 * s))
            path.line(to: NSPoint(x: x + 10 * s, y: y + 40 * s))
            path.move(to: NSPoint(x: x + 50 * s, y: y + 40 * s))
            path.line(to: NSPoint(x: x + 40 * s, y: y + 40 * s))
            path.move(to: NSPoint(x: x, y: y + 10 * s))
            path.line(to: NSPoint(x: x + 10 * s, y: y + 10 * s))
            path.move(to: NSPoint(x: x + 40 * s, y: y + 10 * s))
            path.line(to: NSPoint(x: x + 50 * s, y: y + 10 * s))
            path.move(to: NSPoint(x: x + 10 * s, y: y + 50 * s))
            path.line(to: NSPoint(x: x + 10 * s, y: y + 30 * s))
            path.line(to: NSPoint(x: x + 20 * s, y: y + 30 * s))
            path.line(to: NSPoint(x: x + 20 * s, y: y + 20 * s))
            path.line(to: NSPoint(x: x + 30 * s, y: y + 20 * s))
            path.line(to: NSPoint(x: x + 30 * s, y: y + 40 * s))
            path.line(to: NSPoint(x: x + 40 * s, y: y + 40 * s))
            path.line(to: NSPoint(x: x + 40 * s, y: y + 50 * s))
        case AK_GAME_TILE_FIELD.rawValue:
            path.appendRect(NSRect(x: x + 20 * s, y: y + 20 * s, width: 10 * s, height: 10 * s))
        case AK_GAME_TILE_TOWN.rawValue:
            path.move(to: NSPoint(x: x + 10 * s, y: y + 40 * s))
            path.line(to: NSPoint(x: x + 20 * s, y: y + 40 * s))
            path.line(to: NSPoint(x: x + 20 * s, y: y + 10 * s))
            path.line(to: NSPoint(x: x + 10 * s, y: y + 10 * s))
            path.line(to: NSPoint(x: x + 10 * s, y: y + 20 * s))
            path.line(to: NSPoint(x: x + 40 * s, y: y + 20 * s))
            path.line(to: NSPoint(x: x + 40 * s, y: y + 10 * s))
            path.line(to: NSPoint(x: x + 30 * s, y: y + 10 * s))
            path.line(to: NSPoint(x: x + 30 * s, y: y + 40 * s))
            path.line(to: NSPoint(x: x + 40 * s, y: y + 40 * s))
            path.line(to: NSPoint(x: x + 40 * s, y: y + 30 * s))
            path.line(to: NSPoint(x: x + 10 * s, y: y + 30 * s))
            path.line(to: NSPoint(x: x + 10 * s, y: y + 40 * s))
        case AK_GAME_TILE_DUNGEON.rawValue:
            path.move(to: NSPoint(x: x + 20 * s, y: y + 20 * s))
            path.line(to: NSPoint(x: x + 30 * s, y: y + 30 * s))
            path.move(to: NSPoint(x: x + 20 * s, y: y + 30 * s))
            path.line(to: NSPoint(x: x + 30 * s, y: y + 20 * s))
        case AK_GAME_TILE_CASTLE.rawValue:
            path.appendRect(NSRect(x: x, y: y, width: 50 * s, height: 50 * s))
            path.appendRect(NSRect(x: x + 10 * s, y: y + 10 * s, width: 30 * s, height: 30 * s))
            path.move(to: NSPoint(x: x + 10 * s, y: y + 10 * s))
            path.line(to: NSPoint(x: x + 40 * s, y: y + 40 * s))
            path.move(to: NSPoint(x: x + 10 * s, y: y + 40 * s))
            path.line(to: NSPoint(x: x + 40 * s, y: y + 10 * s))
        default:
            return
        }

        displayColor.setStroke()
        path.lineWidth = max(1.0, s)
        path.stroke()
    }

    private func drawDungeonTile(_ command: AkRenderCommand) {
        let center = point(x: command.x, y: command.y)
        let rect = rectAround(center: center, width: 42, height: 34)
        let path = NSBezierPath()
        let s = pixelScale

        switch UInt32(command.value) {
        case AK_GAME_DUNGEON_WALL.rawValue:
            path.appendRect(rect)
            path.move(to: NSPoint(x: rect.minX, y: rect.maxY))
            path.line(to: NSPoint(x: rect.maxX, y: rect.minY))
        case AK_GAME_DUNGEON_CHEST.rawValue:
            path.appendRect(rect)
            path.move(to: NSPoint(x: rect.minX, y: rect.midY))
            path.line(to: NSPoint(x: rect.minX + 10 * s, y: rect.maxY + 8 * s))
            path.line(to: NSPoint(x: rect.maxX + 10 * s, y: rect.maxY + 8 * s))
            path.line(to: NSPoint(x: rect.maxX, y: rect.midY))
            path.move(to: NSPoint(x: rect.maxX, y: rect.maxY))
            path.line(to: NSPoint(x: rect.maxX + 10 * s, y: rect.maxY + 8 * s))
        case AK_GAME_DUNGEON_LADDER_DOWN.rawValue, AK_GAME_DUNGEON_LADDER_UP.rawValue, AK_GAME_DUNGEON_LADDER_DOWN_ALT.rawValue:
            path.move(to: NSPoint(x: rect.minX + 10 * s, y: rect.minY))
            path.line(to: NSPoint(x: rect.minX + 10 * s, y: rect.maxY))
            path.move(to: NSPoint(x: rect.maxX - 10 * s, y: rect.minY))
            path.line(to: NSPoint(x: rect.maxX - 10 * s, y: rect.maxY))
            for offset in stride(from: 6.0, through: 30.0, by: 8.0) {
                path.move(to: NSPoint(x: rect.minX + 10 * s, y: rect.minY + CGFloat(offset) * s))
                path.line(to: NSPoint(x: rect.maxX - 10 * s, y: rect.minY + CGFloat(offset) * s))
            }
        case AK_GAME_DUNGEON_TRAP.rawValue:
            path.move(to: NSPoint(x: rect.minX, y: rect.minY))
            path.line(to: NSPoint(x: rect.midX, y: rect.maxY))
            path.line(to: NSPoint(x: rect.maxX, y: rect.minY))
        default:
            return
        }

        displayColor.setStroke()
        path.lineWidth = max(1.0, s)
        path.stroke()
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
