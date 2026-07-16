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

public enum AkalabethPrompt: String, Codable, Equatable, Sendable {
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

public struct AkalabethSavedSession: Codable, Equatable, Sendable {
    public var state: AkalabethSavedGameState
    public var inputBuffer: String
    public var statusLine: String
    public var prompt: AkalabethPrompt

    public init(
        state: AkalabethSavedGameState,
        inputBuffer: String = "",
        statusLine: String = "",
        prompt: AkalabethPrompt = .none
    ) {
        self.state = state
        self.inputBuffer = inputBuffer
        self.statusLine = statusLine
        self.prompt = prompt
    }
}

public struct AkalabethSavedGameState: Codable, Equatable, Sendable {
    public var mode: UInt32
    public var location: UInt32
    public var facing: UInt32
    public var playerClass: UInt32
    public var luckyNumber: Int32
    public var levelOfPlay: Int32
    public var stats: [Int32]
    public var inventory: [Int32]
    public var overworld: [Int32]
    public var dungeon: [Int32]
    public var monsterActive: [Int32]
    public var monsterX: [Int32]
    public var monsterY: [Int32]
    public var monsterHitPoints: [Int32]
    public var overworldX: Int32
    public var overworldY: Int32
    public var dungeonX: Int32
    public var dungeonY: Int32
    public var dungeonLevel: Int32
    public var questTarget: Int32
    public var commandCount: Int32
    public var randomState: UInt32
    public var randomLastValue: Double

    public init(
        mode: UInt32,
        location: UInt32,
        facing: UInt32,
        playerClass: UInt32,
        luckyNumber: Int32,
        levelOfPlay: Int32,
        stats: [Int32],
        inventory: [Int32],
        overworld: [Int32],
        dungeon: [Int32],
        monsterActive: [Int32],
        monsterX: [Int32],
        monsterY: [Int32],
        monsterHitPoints: [Int32],
        overworldX: Int32,
        overworldY: Int32,
        dungeonX: Int32,
        dungeonY: Int32,
        dungeonLevel: Int32,
        questTarget: Int32,
        commandCount: Int32,
        randomState: UInt32,
        randomLastValue: Double
    ) {
        self.mode = mode
        self.location = location
        self.facing = facing
        self.playerClass = playerClass
        self.luckyNumber = luckyNumber
        self.levelOfPlay = levelOfPlay
        self.stats = stats
        self.inventory = inventory
        self.overworld = overworld
        self.dungeon = dungeon
        self.monsterActive = monsterActive
        self.monsterX = monsterX
        self.monsterY = monsterY
        self.monsterHitPoints = monsterHitPoints
        self.overworldX = overworldX
        self.overworldY = overworldY
        self.dungeonX = dungeonX
        self.dungeonY = dungeonY
        self.dungeonLevel = dungeonLevel
        self.questTarget = questTarget
        self.commandCount = commandCount
        self.randomState = randomState
        self.randomLastValue = randomLastValue
    }
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

    public convenience init?(savedSession: AkalabethSavedSession) {
        guard let state = Self.restoreState(from: savedSession.state) else {
            return nil
        }
        self.init(
            state: state,
            inputBuffer: savedSession.inputBuffer,
            statusLine: savedSession.statusLine,
            prompt: savedSession.prompt
        )
    }

    public func savedSession() -> AkalabethSavedSession {
        AkalabethSavedSession(
            state: AkalabethSavedGameState(
                mode: state.mode.rawValue,
                location: state.location.rawValue,
                facing: state.facing.rawValue,
                playerClass: state.player_class.rawValue,
                luckyNumber: state.lucky_number,
                levelOfPlay: state.level_of_play,
                stats: Self.flatten(&state.stats, count: Int(AK_GAME_STAT_COUNT)),
                inventory: Self.flatten(&state.inventory, count: Int(AK_GAME_ITEM_COUNT)),
                overworld: Self.flatten(&state.overworld, count: Int(AK_GAME_OVERWORLD_SIZE * AK_GAME_OVERWORLD_SIZE)),
                dungeon: Self.flatten(&state.dungeon, count: Int(AK_GAME_DUNGEON_SIZE * AK_GAME_DUNGEON_SIZE)),
                monsterActive: Self.flatten(&state.monster_active, count: Int(AK_GAME_MONSTER_COUNT + 1)),
                monsterX: Self.flatten(&state.monster_x, count: Int(AK_GAME_MONSTER_COUNT + 1)),
                monsterY: Self.flatten(&state.monster_y, count: Int(AK_GAME_MONSTER_COUNT + 1)),
                monsterHitPoints: Self.flatten(&state.monster_hit_points, count: Int(AK_GAME_MONSTER_COUNT + 1)),
                overworldX: state.overworld_x,
                overworldY: state.overworld_y,
                dungeonX: state.dungeon_x,
                dungeonY: state.dungeon_y,
                dungeonLevel: state.dungeon_level,
                questTarget: state.quest_target,
                commandCount: state.command_count,
                randomState: state.random.state,
                randomLastValue: state.random.last_value
            ),
            inputBuffer: inputBuffer,
            statusLine: statusLine,
            prompt: prompt
        )
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

        if character == " " &&
            (state.mode == AK_GAME_MODE_QUEST || state.mode == AK_GAME_MODE_VICTORY || state.mode == AK_GAME_MODE_DEATH) {
            return applySimpleCommand(AK_GAME_COMMAND_ACKNOWLEDGE)
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

    private static func flatten<T>(_ value: inout T, count: Int) -> [Int32] {
        withUnsafePointer(to: &value) { pointer in
            pointer.withMemoryRebound(to: Int32.self, capacity: count) { cells in
                Array(UnsafeBufferPointer(start: cells, count: count))
            }
        }
    }

    private static func restoreState(from saved: AkalabethSavedGameState) -> AkGameState? {
        let monsterSlots = Int(AK_GAME_MONSTER_COUNT + 1)
        guard saved.stats.count == Int(AK_GAME_STAT_COUNT),
              saved.inventory.count == Int(AK_GAME_ITEM_COUNT),
              saved.overworld.count == Int(AK_GAME_OVERWORLD_SIZE * AK_GAME_OVERWORLD_SIZE),
              saved.dungeon.count == Int(AK_GAME_DUNGEON_SIZE * AK_GAME_DUNGEON_SIZE),
              saved.monsterActive.count == monsterSlots,
              saved.monsterX.count == monsterSlots,
              saved.monsterY.count == monsterSlots,
              saved.monsterHitPoints.count == monsterSlots,
              saved.mode <= AK_GAME_MODE_VICTORY.rawValue,
              saved.location <= AK_GAME_LOCATION_DEATH.rawValue,
              saved.facing <= AK_GAME_DIRECTION_WEST.rawValue,
              saved.playerClass <= AK_GAME_CLASS_MAGE.rawValue
        else {
            return nil
        }

        let mode = AkGameMode(rawValue: saved.mode)
        let location = AkGameLocation(rawValue: saved.location)
        let facing = AkGameDirection(rawValue: saved.facing)
        let playerClass = AkGameClass(rawValue: saved.playerClass)
        var restored = AkGameState()
        ak_game_init(&restored)
        restored.mode = mode
        restored.location = location
        restored.facing = facing
        restored.player_class = playerClass
        restored.lucky_number = saved.luckyNumber
        restored.level_of_play = saved.levelOfPlay
        restore(saved.stats, into: &restored.stats)
        restore(saved.inventory, into: &restored.inventory)
        restore(saved.overworld, into: &restored.overworld)
        restore(saved.dungeon, into: &restored.dungeon)
        restore(saved.monsterActive, into: &restored.monster_active)
        restore(saved.monsterX, into: &restored.monster_x)
        restore(saved.monsterY, into: &restored.monster_y)
        restore(saved.monsterHitPoints, into: &restored.monster_hit_points)
        restored.overworld_x = saved.overworldX
        restored.overworld_y = saved.overworldY
        restored.dungeon_x = saved.dungeonX
        restored.dungeon_y = saved.dungeonY
        restored.dungeon_level = saved.dungeonLevel
        restored.quest_target = saved.questTarget
        restored.command_count = saved.commandCount
        restored.random.state = saved.randomState
        restored.random.last_value = saved.randomLastValue
        return restored
    }

    private static func restore<T>(_ values: [Int32], into value: inout T) {
        withUnsafeMutablePointer(to: &value) { pointer in
            pointer.withMemoryRebound(to: Int32.self, capacity: values.count) { cells in
                for index in values.indices {
                    cells[index] = values[index]
                }
            }
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
