import Testing
import AkalabethMac
import CAkalabeth

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
