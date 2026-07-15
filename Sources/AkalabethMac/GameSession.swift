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

public enum AkalabethPrompt: Equatable, Sendable {
    case none
    case attackWeapon
    case axeStyle
    case magicChoice
    case questName
    case questConsent
    case acknowledgement
}

public enum AkalabethFeedback: Equatable, Sendable {
    case none
    case movement
    case blocked
    case combat
    case purchase
    case death
    case victory
}

public final class GameSession {
    public private(set) var state: AkGameState
    public private(set) var inputBuffer = ""
    public private(set) var statusLine = ""
    public private(set) var prompt: AkalabethPrompt = .none
    public private(set) var lastFeedback: AkalabethFeedback = .none
    private var pendingAttackItem: AkGameItem = AK_GAME_ITEM_FOOD

    public init(fixture: AkalabethFixture = .normal) {
        var initial = AkGameState()
        ak_game_init(&initial)
        self.state = initial
        applyFixture(fixture)
    }

    public init(state: AkGameState, inputBuffer: String = "", statusLine: String = "", prompt: AkalabethPrompt = .none) {
        self.state = state
        self.inputBuffer = inputBuffer
        self.statusLine = statusLine
        self.prompt = prompt
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
        prompt = .none
        lastFeedback = .none
        pendingAttackItem = AK_GAME_ITEM_FOOD
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
        let code = applyCoreCommand(&command)
        statusLine = status(for: code)
        prompt = promptAfterApplying(command: type, code: code)
        return code
    }

    @discardableResult
    public func buy(_ item: AkGameItem) -> AkGameResultCode {
        var command = AkGameCommand(type: AK_GAME_COMMAND_BUY_ITEM, value: 0, item: item, player_class: AK_GAME_CLASS_NONE)
        let code = applyCoreCommand(&command)
        prompt = .none
        statusLine = status(for: code)
        return code
    }

    @discardableResult
    public func attack(with item: AkGameItem) -> AkGameResultCode {
        var command = AkGameCommand(type: AK_GAME_COMMAND_ATTACK, value: 0, item: item, player_class: AK_GAME_CLASS_NONE)
        let code = applyCoreCommand(&command)
        prompt = .none
        statusLine = status(for: code)
        return code
    }

    @discardableResult
    public func castMagic(choice: Int32) -> AkGameResultCode {
        var command = AkGameCommand(type: AK_GAME_COMMAND_CAST_MAGIC, value: choice, item: AK_GAME_ITEM_MAGIC_AMULET, player_class: AK_GAME_CLASS_NONE)
        let code = applyCoreCommand(&command)
        prompt = .none
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
        if prompt != .none {
            return handlePromptCharacter(character)
        }

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
            if state.mode == AK_GAME_MODE_DUNGEON {
                if state.player_class == AK_GAME_CLASS_MAGE {
                    prompt = .magicChoice
                    statusLine = "1-LADDER-UP 2-LADDER-DN 3-KILL 4-BAD??"
                    return AK_GAME_RESULT_OK
                }
                return castMagic(choice: 1)
            }
            return AK_GAME_RESULT_INVALID_COMMAND
        case "R":
            if state.mode == AK_GAME_MODE_TOWN {
                return buy(AK_GAME_ITEM_RAPIER)
            }
            return attack(with: AK_GAME_ITEM_RAPIER)
        case "A":
            if state.mode == AK_GAME_MODE_TOWN {
                return buy(AK_GAME_ITEM_AXE)
            }
            if state.mode == AK_GAME_MODE_DUNGEON {
                prompt = .attackWeapon
                statusLine = "WHICH WEAPON"
                return AK_GAME_RESULT_OK
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
        if prompt == .none && state.mode == AK_GAME_MODE_QUEST && state.quest_target == 0 {
            prompt = .questName
            statusLine = "WHAT IS THY NAME PEASANT"
            return AK_GAME_RESULT_OK
        }
        if prompt == .questName {
            inputBuffer = ""
            prompt = .questConsent
            statusLine = "DOEST THOU WISH FOR GRAND ADVENTURE?"
            return AK_GAME_RESULT_OK
        }
        if prompt == .acknowledgement {
            prompt = .none
            return AK_GAME_RESULT_OK
        }

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
        let code = applyCoreCommand(&command)
        statusLine = status(for: code)
        prompt = promptAfterApplying(command: type, code: code)
        return code
    }

    private var isNumericPrompt: Bool {
        state.mode == AK_GAME_MODE_STARTUP_LUCKY_NUMBER || state.mode == AK_GAME_MODE_STARTUP_LEVEL
    }

    private func selectClass(_ playerClass: AkGameClass) -> AkGameResultCode {
        var command = AkGameCommand(type: AK_GAME_COMMAND_SELECT_CLASS, value: 0, item: AK_GAME_ITEM_FOOD, player_class: playerClass)
        let code = applyCoreCommand(&command)
        statusLine = status(for: code)
        prompt = promptAfterApplying(command: AK_GAME_COMMAND_SELECT_CLASS, code: code)
        return code
    }

    private func handlePromptCharacter(_ character: Character) -> AkGameResultCode {
        switch prompt {
        case .attackWeapon:
            return handleAttackWeapon(character)
        case .axeStyle:
            return handleAxeStyle(character)
        case .magicChoice:
            if let choice = Int32(String(character)), choice >= 1 && choice <= 4 {
                return castMagic(choice: choice)
            }
            statusLine = "CHOICE 1-4"
            return AK_GAME_RESULT_INVALID_COMMAND
        case .questName:
            if character.isLetter || character.isNumber || character == " " {
                inputBuffer.append(character)
                statusLine = inputBuffer
                return AK_GAME_RESULT_OK
            }
            statusLine = "WHAT IS THY NAME PEASANT"
            return AK_GAME_RESULT_INVALID_COMMAND
        case .questConsent:
            if character == "Y" {
                prompt = .none
                inputBuffer = ""
                return applySimpleCommand(AK_GAME_COMMAND_ACKNOWLEDGE)
            }
            prompt = .acknowledgement
            inputBuffer = ""
            statusLine = "THEN LEAVE AND BEGONE!"
            return AK_GAME_RESULT_OK
        case .acknowledgement:
            if character == " " {
                prompt = .none
                statusLine = ""
                return AK_GAME_RESULT_OK
            }
            statusLine = "PRESS -SPACE- TO CONT."
            return AK_GAME_RESULT_INVALID_COMMAND
        case .none:
            return AK_GAME_RESULT_INVALID_COMMAND
        }
    }

    private func handleAttackWeapon(_ character: Character) -> AkGameResultCode {
        switch character {
        case "R":
            return attack(with: AK_GAME_ITEM_RAPIER)
        case "A":
            pendingAttackItem = AK_GAME_ITEM_AXE
            prompt = .axeStyle
            statusLine = "TO THROW OR SWING"
            return AK_GAME_RESULT_OK
        case "S":
            return attack(with: AK_GAME_ITEM_SHIELD)
        case "B":
            return attack(with: AK_GAME_ITEM_BOW)
        case "M":
            if state.player_class == AK_GAME_CLASS_MAGE {
                prompt = .magicChoice
                statusLine = "1-LADDER-UP 2-LADDER-DN 3-KILL 4-BAD??"
                return AK_GAME_RESULT_OK
            }
            return castMagic(choice: 1)
        default:
            statusLine = "WHICH WEAPON"
            return AK_GAME_RESULT_INVALID_COMMAND
        }
    }

    private func handleAxeStyle(_ character: Character) -> AkGameResultCode {
        if character == "T" {
            if state.inventory.2 < 1 {
                prompt = .attackWeapon
                statusLine = "NOT OWNED"
                return AK_GAME_RESULT_REJECTED
            }
            state.inventory.2 -= 1
            return attack(with: AK_GAME_ITEM_BOW)
        }
        return attack(with: pendingAttackItem)
    }

    private func promptAfterApplying(command: AkGameCommandType, code: AkGameResultCode) -> AkalabethPrompt {
        guard code == AK_GAME_RESULT_OK else {
            return prompt
        }
        if command == AK_GAME_COMMAND_ENTER && state.mode == AK_GAME_MODE_QUEST && state.quest_target == 0 {
            statusLine = "WHAT IS THY NAME PEASANT"
            return .questName
        }
        if state.mode == AK_GAME_MODE_QUEST || state.mode == AK_GAME_MODE_VICTORY || state.mode == AK_GAME_MODE_DEATH {
            return .none
        }
        return .none
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

    private func applyCoreCommand(_ command: inout AkGameCommand) -> AkGameResultCode {
        let before = state
        let code = ak_game_apply_command(&state, &command, nil)
        lastFeedback = Self.feedback(for: command.type, code: code, before: before, after: state)
        return code
    }

    private static func feedback(
        for command: AkGameCommandType,
        code: AkGameResultCode,
        before: AkGameState,
        after: AkGameState
    ) -> AkalabethFeedback {
        if after.mode == AK_GAME_MODE_VICTORY && before.mode != AK_GAME_MODE_VICTORY {
            return .victory
        }
        if after.mode == AK_GAME_MODE_DEATH && before.mode != AK_GAME_MODE_DEATH {
            return .death
        }
        if code == AK_GAME_RESULT_REJECTED || code == AK_GAME_RESULT_INVALID_COMMAND || code == AK_GAME_RESULT_INVALID_ARGUMENT {
            return .blocked
        }
        if command == AK_GAME_COMMAND_BUY_ITEM && code == AK_GAME_RESULT_OK {
            return .purchase
        }
        if (command == AK_GAME_COMMAND_ATTACK || command == AK_GAME_COMMAND_CAST_MAGIC) && code == AK_GAME_RESULT_OK {
            return .combat
        }
        if isMovementCommand(command) && code == AK_GAME_RESULT_OK {
            if before.location == AK_GAME_LOCATION_OVERWORLD {
                return before.overworld_x == after.overworld_x && before.overworld_y == after.overworld_y ? .blocked : .movement
            }
            if before.location == AK_GAME_LOCATION_DUNGEON {
                if command == AK_GAME_COMMAND_TURN_LEFT || command == AK_GAME_COMMAND_TURN_RIGHT || command == AK_GAME_COMMAND_TURN_AROUND {
                    return .movement
                }
                return before.dungeon_x == after.dungeon_x && before.dungeon_y == after.dungeon_y ? .blocked : .movement
            }
            return .movement
        }
        return .none
    }

    private static func isMovementCommand(_ command: AkGameCommandType) -> Bool {
        command == AK_GAME_COMMAND_MOVE_NORTH ||
            command == AK_GAME_COMMAND_MOVE_EAST ||
            command == AK_GAME_COMMAND_MOVE_SOUTH ||
            command == AK_GAME_COMMAND_MOVE_WEST ||
            command == AK_GAME_COMMAND_FORWARD ||
            command == AK_GAME_COMMAND_TURN_LEFT ||
            command == AK_GAME_COMMAND_TURN_RIGHT ||
            command == AK_GAME_COMMAND_TURN_AROUND ||
            command == AK_GAME_COMMAND_ENTER ||
            command == AK_GAME_COMMAND_EXIT
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
