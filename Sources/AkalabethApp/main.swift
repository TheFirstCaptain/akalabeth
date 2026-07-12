import AppKit
import AkalabethMac
import CAkalabeth
import Darwin

@MainActor
final class AppDelegate: NSObject, NSApplicationDelegate {
    private var window: NSWindow?
    private var gameView: GameView?
    private var session = GameSession(fixture: AppDelegate.fixtureFromArguments())

    func applicationDidFinishLaunching(_ notification: Notification) {
        buildMenu()
        buildWindow()
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool {
        true
    }

    @objc private func newGame() {
        session.reset()
        gameView?.session = session
    }

    @objc private func resetGame() {
        session.reset(fixture: AppDelegate.fixtureFromArguments())
        gameView?.session = session
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
        let view = GameView(frame: NSRect(x: 0, y: 0, width: 840, height: 600))
        view.session = session
        gameView = view

        let window = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 840, height: 600),
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
        gameMenu.addItem(.separator())
        gameMenu.addItem(withTitle: "Debug Town Fixture", action: #selector(launchTownFixture), keyEquivalent: "1").target = self
        gameMenu.addItem(withTitle: "Debug Overworld Fixture", action: #selector(launchOverworldFixture), keyEquivalent: "2").target = self
        gameMenu.addItem(withTitle: "Debug Dungeon Fixture", action: #selector(launchDungeonFixture), keyEquivalent: "3").target = self
        gameMenu.addItem(withTitle: "Debug Quest Fixture", action: #selector(launchQuestFixture), keyEquivalent: "4").target = self
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

    private let textFont = NSFont.monospacedSystemFont(ofSize: 18, weight: .regular)
    private let smallFont = NSFont.monospacedSystemFont(ofSize: 14, weight: .regular)

    override var acceptsFirstResponder: Bool {
        true
    }

    override func keyDown(with event: NSEvent) {
        if let input = input(from: event) {
            session.handle(input)
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
        NSColor.systemGreen.setStroke()
        path.lineWidth = 2
        path.stroke()
    }

    private func drawTile(_ command: AkRenderCommand, scaleX: CGFloat, scaleY: CGFloat) {
        let center = point(x: command.x, y: command.y, scaleX: scaleX, scaleY: scaleY)
        let rect = NSRect(x: center.x - 28, y: center.y - 18, width: 56, height: 36)
        NSColor.systemGreen.setStroke()
        NSBezierPath(rect: rect).stroke()
        drawString(commandText(command).uppercased(), at: NSPoint(x: rect.minX + 5, y: rect.midY - 7), font: smallFont, inverse: false)
    }

    private func drawCreature(_ command: AkRenderCommand, scaleX: CGFloat, scaleY: CGFloat) {
        let center = point(x: command.x, y: command.y, scaleX: scaleX, scaleY: scaleY)
        let rect = NSRect(x: center.x - 46, y: center.y - 28, width: 92, height: 56)
        NSColor.systemGreen.setStroke()
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
            .foregroundColor: inverse ? NSColor.black : NSColor.systemGreen,
            .backgroundColor: inverse ? NSColor.systemGreen : NSColor.clear
        ]
        text.draw(at: point, withAttributes: attributes)
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
