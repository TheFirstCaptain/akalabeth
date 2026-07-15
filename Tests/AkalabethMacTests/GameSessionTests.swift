import Testing
import AkalabethMac
import CAkalabeth
import Foundation

@Test func startupInputAdvancesToTown() {
    let session = GameSession()

    #expect(session.state.mode == AK_GAME_MODE_STARTUP_LUCKY_NUMBER)
    #expect(session.handle(.character("1")) == AK_GAME_RESULT_OK)
    #expect(session.handle(.character("2")) == AK_GAME_RESULT_OK)
    #expect(session.handle(.enter) == AK_GAME_RESULT_OK)
    #expect(session.state.mode == AK_GAME_MODE_STARTUP_LEVEL)

    #expect(session.handle(.character("5")) == AK_GAME_RESULT_OK)
    #expect(session.handle(.enter) == AK_GAME_RESULT_OK)
    #expect(session.state.mode == AK_GAME_MODE_CHARACTER_REVIEW)

    #expect(session.handle(.character("Y")) == AK_GAME_RESULT_OK)
    #expect(session.state.mode == AK_GAME_MODE_CLASS_SELECT)
    #expect(session.handle(.character("F")) == AK_GAME_RESULT_OK)
    #expect(session.state.mode == AK_GAME_MODE_TOWN)
}

@Test func fixturesLaunchRepresentativeModes() {
    #expect(GameSession(fixture: .town).state.mode == AK_GAME_MODE_TOWN)
    #expect(GameSession(fixture: .overworld).state.mode == AK_GAME_MODE_OVERWORLD)
    #expect(GameSession(fixture: .dungeon).state.mode == AK_GAME_MODE_DUNGEON)
    #expect(GameSession(fixture: .quest).state.mode == AK_GAME_MODE_QUEST)
}

@Test func townKeyMappingBuysAndExits() {
    let session = GameSession(fixture: .town)
    let foodBefore = session.state.inventory.0

    #expect(session.handle(.character("F")) == AK_GAME_RESULT_OK)
    #expect(session.state.inventory.0 > foodBefore)
    #expect(session.handle(.character("Q")) == AK_GAME_RESULT_OK)
    #expect(session.state.mode == AK_GAME_MODE_OVERWORLD)
}

@Test func dungeonInputMapsToCoreCommands() {
    let session = GameSession(fixture: .dungeon)
    let facing = session.state.facing

    #expect(session.handle(.right) == AK_GAME_RESULT_OK)
    #expect(session.state.facing != facing)
    #expect(session.lastFeedback == .movement)
    #expect(session.handle(.character("A")) == AK_GAME_RESULT_OK)
    #expect(session.prompt == .attackWeapon)
    #expect(session.handle(.character("R")) == AK_GAME_RESULT_OK)
    #expect(session.prompt == .none)
    #expect(session.lastFeedback == .combat)
}

@Test func axeAttackAsksForThrowOrSwing() {
    let session = GameSession(fixture: .dungeon)

    #expect(session.handle(.character("A")) == AK_GAME_RESULT_OK)
    #expect(session.prompt == .attackWeapon)
    #expect(session.handle(.character("A")) == AK_GAME_RESULT_OK)
    #expect(session.prompt == .axeStyle)
    #expect(session.handle(.character("S")) == AK_GAME_RESULT_OK)
    #expect(session.prompt == .none)
}

@Test func mageMagicAsksForChoice() {
    var state = GameSession(fixture: .dungeon).state
    state.player_class = AK_GAME_CLASS_MAGE
    let session = GameSession(state: state)

    #expect(session.handle(.character("M")) == AK_GAME_RESULT_OK)
    #expect(session.prompt == .magicChoice)
    #expect(session.handle(.character("2")) == AK_GAME_RESULT_OK)
    #expect(session.prompt == .none)
    #expect(session.state.dungeon.5.5 == AK_GAME_DUNGEON_LADDER_DOWN.rawValue)
}

@Test func firstQuestEntryPromptsForNameAndConsent() {
    var state = GameSession(fixture: .quest).state
    state.quest_target = 0
    let session = GameSession(state: state)

    #expect(session.handle(.enter) == AK_GAME_RESULT_OK)
    #expect(session.prompt == .questName)
    #expect(session.handle(.character("A")) == AK_GAME_RESULT_OK)
    #expect(session.inputBuffer == "A")
    #expect(session.handle(.enter) == AK_GAME_RESULT_OK)
    #expect(session.prompt == .questConsent)
    #expect(session.handle(.character("Y")) == AK_GAME_RESULT_OK)
    #expect(session.prompt == .none)
    #expect(session.state.quest_target > 0)
}

@Test func renderBufferAvailableForAppShell() {
    let session = GameSession(fixture: .quest)
    let buffer = session.renderCommands()

    #expect(buffer.count > 0)
    #expect(buffer.commands.0.type == AK_RENDER_COMMAND_SET_MODE)
    #expect(buffer.commands.0.mode == AK_RENDER_MODE_TEXT)
}

@Test func sessionSnapshotRestoresCoreState() throws {
    let session = GameSession(fixture: .dungeon)

    let restored = try #require(GameSession(snapshot: session.snapshot()))

    #expect(restored.state.mode == AK_GAME_MODE_DUNGEON)
    #expect(restored.state.location == AK_GAME_LOCATION_DUNGEON)
    #expect(restored.state.dungeon_x == session.state.dungeon_x)
    #expect(restored.state.dungeon_y == session.state.dungeon_y)
    #expect(restored.state.inventory.0 == session.state.inventory.0)
}

@Test func sessionFeedbackMapsPurchasesBlocksDeathAndVictory() {
    let town = GameSession(fixture: .town)
    #expect(town.handle(.character("F")) == AK_GAME_RESULT_OK)
    #expect(town.lastFeedback == .purchase)

    var blockedState = GameSession(fixture: .dungeon).state
    blockedState.facing = AK_GAME_DIRECTION_EAST
    blockedState.dungeon.6.5 = Int32(AK_GAME_DUNGEON_WALL.rawValue)
    let blocked = GameSession(state: blockedState)
    #expect(blocked.handle(.character("F")) == AK_GAME_RESULT_OK)
    #expect(blocked.lastFeedback == .blocked)

    var deathState = GameSession(fixture: .overworld).state
    deathState.inventory.0 = 0
    deathState.stats.0 = 1
    let death = GameSession(state: deathState)
    #expect(death.handle(.character("P")) == AK_GAME_RESULT_OK)
    #expect(death.lastFeedback == .death)

    var victoryState = GameSession(fixture: .quest).state
    victoryState.quest_target = -10
    let victory = GameSession(state: victoryState)
    #expect(victory.handle(.enter) == AK_GAME_RESULT_OK)
    #expect(victory.lastFeedback == .victory)
}

@Test func persistenceStoresSettingsAndSaveData() throws {
    let directory = FileManager.default.temporaryDirectory
        .appendingPathComponent("AkalabethPersistenceTests-\(UUID().uuidString)", isDirectory: true)
    let suiteName = "AkalabethPersistenceTests.\(UUID().uuidString)"
    let defaults = try #require(UserDefaults(suiteName: suiteName))
    defaults.removePersistentDomain(forName: suiteName)
    let persistence = AkalabethPersistence(applicationSupportDirectory: directory, defaults: defaults)
    defer {
        try? FileManager.default.removeItem(at: directory)
        defaults.removePersistentDomain(forName: suiteName)
    }

    persistence.saveSettings(AkalabethSettings(
        colorTreatment: .amber,
        windowScale: 3,
        integerScaling: false,
        highContrast: true,
        scanlines: true,
        audioEnabled: true
    ))
    #expect(persistence.loadSettings() == AkalabethSettings(
        colorTreatment: .amber,
        windowScale: 3,
        integerScaling: false,
        highContrast: true,
        scanlines: true,
        audioEnabled: true
    ))

    let session = GameSession(fixture: .quest)
    try persistence.saveSession(session)
    let resumed = try #require(try persistence.resumeSession())

    #expect(resumed.state.mode == AK_GAME_MODE_QUEST)
    #expect(resumed.state.quest_target == session.state.quest_target)

    try persistence.deleteSave()
    #expect(try persistence.resumeSession() == nil)
}
