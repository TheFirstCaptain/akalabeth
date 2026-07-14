import Foundation
import CAkalabeth

public enum AkalabethFixture: String, Sendable {
    case normal
    case town
    case overworld
    case dungeon
    case quest
}

public enum AkalabethInput: Equatable, Sendable {
    case character(Character)
    case enter
    case escape
    case up
    case right
    case down
    case left
}

public final class GameSession {
    public private(set) var state: AkGameState
    public private(set) var inputBuffer = ""
    public private(set) var statusLine = ""

    public init(fixture: AkalabethFixture = .normal) {
        var initial = AkGameState()
        ak_game_init(&initial)
        self.state = initial
        applyFixture(fixture)
    }

    public init(state: AkGameState, inputBuffer: String = "", statusLine: String = "") {
        self.state = state
        self.inputBuffer = inputBuffer
        self.statusLine = statusLine
    }

    public convenience init?(snapshot: Data, inputBuffer: String = "", statusLine: String = "") {
        guard snapshot.count == Self.snapshotByteCount else {
            return nil
        }

        var restored = AkGameState()
        _ = withUnsafeMutableBytes(of: &restored) { rawState in
            snapshot.copyBytes(to: rawState)
        }
        self.init(state: restored, inputBuffer: inputBuffer, statusLine: statusLine)
    }

    public static var snapshotByteCount: Int {
        MemoryLayout<AkGameState>.size
    }

    public func snapshot() -> Data {
        withUnsafeBytes(of: state) { rawState in
            Data(rawState)
        }
    }

    public func reset(fixture: AkalabethFixture = .normal) {
        ak_game_init(&state)
        inputBuffer = ""
        statusLine = ""
        applyFixture(fixture)
    }

    @discardableResult
    public func handle(_ input: AkalabethInput) -> AkGameResultCode {
        switch input {
        case .character(let character):
            return handleCharacter(Character(String(character).uppercased()))
        case .enter:
            return submitBufferedInput()
        case .escape:
            reset()
            statusLine = "New game"
            return AK_GAME_RESULT_OK
        case .up:
            return applySimpleCommand(AK_GAME_COMMAND_MOVE_NORTH)
        case .right:
            return applyDirectionalInput(rightCommand())
        case .down:
            return applySimpleCommand(AK_GAME_COMMAND_MOVE_SOUTH)
        case .left:
            return applyDirectionalInput(leftCommand())
        }
    }

    @discardableResult
    public func applySimpleCommand(_ type: AkGameCommandType) -> AkGameResultCode {
        var command = AkGameCommand(type: type, value: 0, item: AK_GAME_ITEM_FOOD, player_class: AK_GAME_CLASS_NONE)
        let code = ak_game_apply_command(&state, &command, nil)
        statusLine = status(for: code)
        return code
    }

    @discardableResult
    public func buy(_ item: AkGameItem) -> AkGameResultCode {
        var command = AkGameCommand(type: AK_GAME_COMMAND_BUY_ITEM, value: 0, item: item, player_class: AK_GAME_CLASS_NONE)
        let code = ak_game_apply_command(&state, &command, nil)
        statusLine = status(for: code)
        return code
    }

    @discardableResult
    public func attack(with item: AkGameItem) -> AkGameResultCode {
        var command = AkGameCommand(type: AK_GAME_COMMAND_ATTACK, value: 0, item: item, player_class: AK_GAME_CLASS_NONE)
        let code = ak_game_apply_command(&state, &command, nil)
        statusLine = status(for: code)
        return code
    }

    @discardableResult
    public func castMagic(choice: Int32) -> AkGameResultCode {
        var command = AkGameCommand(type: AK_GAME_COMMAND_CAST_MAGIC, value: choice, item: AK_GAME_ITEM_MAGIC_AMULET, player_class: AK_GAME_CLASS_NONE)
        let code = ak_game_apply_command(&state, &command, nil)
        statusLine = status(for: code)
        return code
    }

    public func renderCommands() -> AkRenderCommandBuffer {
        var buffer = AkRenderCommandBuffer()
        ak_render_game(&state, &buffer)
        return buffer
    }

    public func setStatusLine(_ text: String) {
        statusLine = text
    }

    private func handleCharacter(_ character: Character) -> AkGameResultCode {
        if isNumericPrompt {
            if character.isNumber {
                inputBuffer.append(character)
                statusLine = inputBuffer
            }
            return AK_GAME_RESULT_OK
        }

        switch character {
        case "Y":
            return applySimpleCommand(AK_GAME_COMMAND_ACCEPT_CHARACTER)
        case "N":
            return applySimpleCommand(AK_GAME_COMMAND_REROLL_CHARACTER)
        case "F":
            if state.mode == AK_GAME_MODE_CLASS_SELECT {
                return selectClass(AK_GAME_CLASS_FIGHTER)
            }
            if state.mode == AK_GAME_MODE_TOWN {
                return buy(AK_GAME_ITEM_FOOD)
            }
            return applySimpleCommand(AK_GAME_COMMAND_FORWARD)
        case "M":
            if state.mode == AK_GAME_MODE_CLASS_SELECT {
                return selectClass(AK_GAME_CLASS_MAGE)
            }
            if state.mode == AK_GAME_MODE_TOWN {
                return buy(AK_GAME_ITEM_MAGIC_AMULET)
            }
            return castMagic(choice: 1)
        case "R":
            if state.mode == AK_GAME_MODE_TOWN {
                return buy(AK_GAME_ITEM_RAPIER)
            }
            return attack(with: AK_GAME_ITEM_RAPIER)
        case "A":
            if state.mode == AK_GAME_MODE_TOWN {
                return buy(AK_GAME_ITEM_AXE)
            }
            return attack(with: AK_GAME_ITEM_AXE)
        case "S":
            if state.mode == AK_GAME_MODE_TOWN {
                return buy(AK_GAME_ITEM_SHIELD)
            }
            return attack(with: AK_GAME_ITEM_SHIELD)
        case "B":
            if state.mode == AK_GAME_MODE_TOWN {
                return buy(AK_GAME_ITEM_BOW)
            }
            return attack(with: AK_GAME_ITEM_BOW)
        case "Q":
            return applySimpleCommand(AK_GAME_COMMAND_EXIT)
        case "P", " ":
            return applySimpleCommand(AK_GAME_COMMAND_PASS)
        case "E":
            return applySimpleCommand(AK_GAME_COMMAND_ENTER)
        case "1", "2", "3", "4":
            return castMagic(choice: Int32(String(character)) ?? 1)
        default:
            statusLine = "Unmapped key"
            return AK_GAME_RESULT_INVALID_COMMAND
        }
    }

    private func submitBufferedInput() -> AkGameResultCode {
        if state.mode == AK_GAME_MODE_QUEST || state.mode == AK_GAME_MODE_VICTORY || state.mode == AK_GAME_MODE_DEATH {
            return applySimpleCommand(AK_GAME_COMMAND_ACKNOWLEDGE)
        }
        guard isNumericPrompt else {
            return applySimpleCommand(AK_GAME_COMMAND_ENTER)
        }
        let value = Int32(inputBuffer) ?? 0
        inputBuffer = ""
        let type = state.mode == AK_GAME_MODE_STARTUP_LUCKY_NUMBER ? AK_GAME_COMMAND_SET_LUCKY_NUMBER : AK_GAME_COMMAND_SET_LEVEL
        var command = AkGameCommand(type: type, value: value, item: AK_GAME_ITEM_FOOD, player_class: AK_GAME_CLASS_NONE)
        let code = ak_game_apply_command(&state, &command, nil)
        statusLine = status(for: code)
        return code
    }

    private var isNumericPrompt: Bool {
        state.mode == AK_GAME_MODE_STARTUP_LUCKY_NUMBER || state.mode == AK_GAME_MODE_STARTUP_LEVEL
    }

    private func selectClass(_ playerClass: AkGameClass) -> AkGameResultCode {
        var command = AkGameCommand(type: AK_GAME_COMMAND_SELECT_CLASS, value: 0, item: AK_GAME_ITEM_FOOD, player_class: playerClass)
        let code = ak_game_apply_command(&state, &command, nil)
        statusLine = status(for: code)
        return code
    }

    private func applyDirectionalInput(_ dungeonCommand: AkGameCommandType) -> AkGameResultCode {
        if state.mode == AK_GAME_MODE_DUNGEON || state.mode == AK_GAME_MODE_COMBAT {
            return applySimpleCommand(dungeonCommand)
        }
        return applySimpleCommand(dungeonCommand == AK_GAME_COMMAND_TURN_RIGHT ? AK_GAME_COMMAND_MOVE_EAST : AK_GAME_COMMAND_MOVE_WEST)
    }

    private func rightCommand() -> AkGameCommandType {
        AK_GAME_COMMAND_TURN_RIGHT
    }

    private func leftCommand() -> AkGameCommandType {
        AK_GAME_COMMAND_TURN_LEFT
    }

    private func applyFixture(_ fixture: AkalabethFixture) {
        switch fixture {
        case .normal:
            return
        case .town:
            seedCharacter()
        case .overworld:
            seedCharacter()
            applySimpleCommand(AK_GAME_COMMAND_EXIT)
        case .dungeon:
            seedCharacter()
            state.mode = AK_GAME_MODE_DUNGEON
            state.location = AK_GAME_LOCATION_DUNGEON
            state.facing = AK_GAME_DIRECTION_EAST
            state.dungeon_level = 1
            state.dungeon_x = 5
            state.dungeon_y = 5
            state.stats.0 = 30
            state.inventory.0 = 20
            state.inventory.1 = 1
            setDungeonTile(x: 6, y: 5, tile: AK_GAME_DUNGEON_CHEST)
        case .quest:
            seedCharacter()
            state.mode = AK_GAME_MODE_QUEST
            state.location = AK_GAME_LOCATION_CASTLE
            state.quest_target = 1
        }
    }

    private func seedCharacter() {
        var lucky = AkGameCommand(type: AK_GAME_COMMAND_SET_LUCKY_NUMBER, value: 1234, item: AK_GAME_ITEM_FOOD, player_class: AK_GAME_CLASS_NONE)
        ak_game_apply_command(&state, &lucky, nil)
        var level = AkGameCommand(type: AK_GAME_COMMAND_SET_LEVEL, value: 5, item: AK_GAME_ITEM_FOOD, player_class: AK_GAME_CLASS_NONE)
        ak_game_apply_command(&state, &level, nil)
        var accept = AkGameCommand(type: AK_GAME_COMMAND_ACCEPT_CHARACTER, value: 0, item: AK_GAME_ITEM_FOOD, player_class: AK_GAME_CLASS_NONE)
        ak_game_apply_command(&state, &accept, nil)
        _ = selectClass(AK_GAME_CLASS_FIGHTER)
        state.inventory.0 = 25
        state.inventory.1 = 1
        state.inventory.2 = 1
        state.inventory.3 = 1
        state.inventory.4 = 1
        state.inventory.5 = 1
    }

    private func setDungeonTile(x: Int, y: Int, tile: AkGameDungeonTile) {
        withUnsafeMutablePointer(to: &state.dungeon) { pointer in
            pointer.withMemoryRebound(to: Int32.self, capacity: Int(AK_GAME_DUNGEON_SIZE * AK_GAME_DUNGEON_SIZE)) { cells in
                cells[x * Int(AK_GAME_DUNGEON_SIZE) + y] = Int32(tile.rawValue)
            }
        }
    }

    private func status(for code: AkGameResultCode) -> String {
        switch code {
        case AK_GAME_RESULT_OK:
            return ""
        case AK_GAME_RESULT_REJECTED:
            return "Rejected"
        case AK_GAME_RESULT_INVALID_COMMAND:
            return "Invalid command"
        case AK_GAME_RESULT_INVALID_ARGUMENT:
            return "Invalid argument"
        case AK_GAME_RESULT_UNSUPPORTED_RULE:
            return "Unsupported rule"
        default:
            return "Unknown result"
        }
    }
}
