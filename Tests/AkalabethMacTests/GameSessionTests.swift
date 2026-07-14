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
    #expect(session.handle(.character("R")) == AK_GAME_RESULT_OK)
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

    persistence.saveSettings(AkalabethSettings(colorTreatment: .amber, windowScale: 3))
    #expect(persistence.loadSettings() == AkalabethSettings(colorTreatment: .amber, windowScale: 3))

    let session = GameSession(fixture: .quest)
    try persistence.saveSession(session)
    let resumed = try #require(try persistence.resumeSession())

    #expect(resumed.state.mode == AK_GAME_MODE_QUEST)
    #expect(resumed.state.quest_target == session.state.quest_target)

    try persistence.deleteSave()
    #expect(try persistence.resumeSession() == nil)
}
