# Akalabeth Modern Mac Port

This repository contains a modernization of Akalabeth, Richard Garriott's pre-Ultima dungeon crawler, built around the original Applesoft BASIC listing in `AKLABETH.TXT`.

The project keeps the historical game flow and rules visible while adding a native macOS shell, a portable C game core, Swift tests, deterministic compatibility fixtures, save/resume support, and display options. The original README text is preserved below under "Original README".

The original 1980/1981 California Pacific manual is available at the Museum of Computer Adventure Game History: <https://www.mocagh.org/origin/akalabeth-manual.pdf>.

## Requirements

- macOS 13 or newer
- Swift 6 toolchain or current Xcode command line tools
- Apple silicon is the primary target for the packaged app

## Run Locally

From the repository root:

```bash
swift build
.build/arm64-apple-macosx/debug/AkalabethApp
```

Useful development launches:

```bash
.build/arm64-apple-macosx/debug/AkalabethApp --smoke-test
.build/arm64-apple-macosx/debug/AkalabethApp --fixture=town
.build/arm64-apple-macosx/debug/AkalabethApp --fixture=overworld
.build/arm64-apple-macosx/debug/AkalabethApp --fixture=dungeon
.build/arm64-apple-macosx/debug/AkalabethApp --fixture=quest
```

To build a release `.app` bundle:

```bash
Scripts/package_macos_app.sh
.build/release/Akalabeth.app/Contents/MacOS/Akalabeth
```

For clean release packaging, signing, notarization, and manual validation steps, see `docs/RELEASE.md`.

## Validation

```bash
swift test
make -C harness test
```

## How To Play

### Manual Context

The original manual frames the game as Lord British's charge to clear Akalabeth of the creatures left by Mondain. It also explains several rules that are easy to miss in a modern port:

- The lucky number is the random seed. Reusing the same number recreates the same initial setup, including the map, dungeons, player generation, and monster placement.
- The level of play scales monster strength. Level 10 monsters are described as ten times harder to destroy than level 1 monsters.
- Hit points are the damage the player can absorb before death. They decrease when monsters hit you and increase when you leave a dungeon after slaying creatures there.
- Strength affects the damage you can inflict.
- Dexterity affects hit probability.
- Stamina affects defensive staying power after sustained combat.
- Wisdom is used by special quest routines.
- Gold is the currency used for supplies and equipment.
- Fighters can use rapiers and bows, but cannot control a magic amulet's effect. Mages can control magic amulets, but cannot use rapiers or bows.
- The game starts in a town on a 20x20 overworld map. The manual strongly warns new players to buy enough food before leaving town.
- The original Apple II controls used `X` for enter, climb, and descend actions; this port maps those actions to `E`.

### Startup

1. Enter a lucky number and press Return. This seeds the world and dungeon layout.
2. Enter a level of play and press Return. Higher levels make monsters stronger.
3. At the character review, press `Y` to accept or `N` to reroll.
4. Choose `F` for fighter or `M` for mage.

### Outside

- Arrow keys move north, east, south, and west.
- `E` enters the current town, dungeon, or castle tile.
- `P` or Space passes a turn.
- Food is consumed as you travel. Running out of food is fatal.

### Town

The town screen is the store and status screen.

- `F` buys food.
- `R` buys a rapier.
- `A` buys an axe.
- `S` buys a shield.
- `B` buys a bow.
- `M` buys a magic amulet.
- `Q` leaves town and returns to the overworld.

### Dungeon

- Up arrow or `F` moves forward.
- Left and Right arrows turn.
- `P` or Space passes a turn.
- `E` enters a ladder when standing on it.
- `A` opens the attack prompt.
- At the attack prompt, choose `R`, `A`, `S`, `B`, or `M`.
- Mages can choose magic effects with `1`, `2`, `3`, or `4`; fighters can use amulets but do not control the effect.

### Castle And Quest

- Enter Lord British's castle with `E`.
- Press Return through quest prompts.
- Complete assigned monster quests and return to the castle for the next quest.

### Modern Mac Conveniences

- Game > Save Game writes a modern save file.
- Game > Resume Saved Game reloads that save.
- Game > New Game clears the save and starts fresh.
- Game > Reset restarts the current launch fixture.
- Settings includes green/amber display, original scale, integer scaling, high contrast, scanline treatment, and optional audio feedback.
- Help > Akalabeth Help summarizes controls and compatibility notes inside the app.

## Project Layout

- `AKLABETH.TXT`: original Applesoft BASIC listing.
- `Core/`: portable C game rules, RNG adapter, and render command model.
- `Sources/AkalabethMac/`: Swift session and persistence adapters.
- `Sources/AkalabethApp/`: native AppKit shell.
- `Tests/`: Swift package tests.
- `harness/`: C characterization and compatibility tests.
- `docs/features/`: feature tracker and implementation notes.

## Original README

AkalaBeth
=========

This is where it all started; a little mini Ultima.  Because this was written
in Basic, there are a lot of variations floating around the net.  The most 
common change was to modify the Magic Amulet.  I believe the listing that 
I have included here is original.

The dungeons are almost the same as what ended up in Ultima.  Each level is 
a 10x10 and the first 3 monsters were put on the first level.  Second level 
has 4, the third has 5, and so forth.  Also, the Giant Rat that you meet on 
the first level has even more hit points on the second!  By the time you hit 
the 8th level you have stuff attacking you from all directions!

Screen Shot's
=============

STARTUP1 GIF - First screen after you start the program.  Lucky number 
               is used to seed the random number generator for world and 
               dungeon layouts. Level of play, makes monsters harder. 
STARTUP2 GIF - Next your attributes are generated.  You just hit "N" until
               you see number you like.  Then you choose to be a fighter or
               mage.  Fighters can use the magic amulet but can't control
               what it does while a mage can't use rapiers or bows.
STARTUP3 GIF - Welcome to Aklabeth screen.  Shows this while generating world.
OUTSIDE  GIF - The world is a 20x20 grid.  When you are outside, you can see
               your square and all adjacent squares.  You are the "+" in the
               middle.  To the south is the symbol for a town.  To the west
               is the symbol for a castle.  SW is a mountain and NE (the 'X')
               is a dungeon.  The entire perimeter of the 20x20 grid is 
               surrounded by mountains.
TOWN     GIF - When you enter a town, you go immediately to the store.  Here
               you stock up on the necessities.  When you exit the store,
               you also exit the town. This screen also doubles as your
               status screen.
LB-1     GIF - This is the message the first time you enter Lord British's
               Castle.  He askes you your name and puts you on a quest.  The
               first quest is in direct relation to your wisdom level.
LB-2     GIF - After completing you quest, Lord British sends you on another.
               Each new quest is harder than the previous.  The very last 
               quest is the Balrog quest.
LB-3     GIF - After completing the last quest, Lord British makes you
               a Lord and then encourages you to try the next difficulty
               level.  If you completed it while at level 10, he give you 
               a number to call to report thy feat!

Monsters (in order of difficulty)
=================================
MNSTR-01 GIF - Skeleton
MNSTR-02 GIF - Thief (50% chance he will steal *any* item from you including
                      the weapon you are using!)
MNSTR-03 GIF - Giant Rat
MNSTR-04 GIF - Orc
MNSTR-05 GIF - Viper
MNSTR-06 GIF - Carrion Crawler
MNSTR-07 GIF - Gremlin (50% chance that he will eat half your food each turn!)
MNSTR-08 GIF - Mimic (will actually sit still in distance to get you to 
                      go to him)
MNSTR-09 GIF - Daemon
MNSTR-10 GIF - Balrog

CHEST    GIF - In the dungeons, you may find chests containing gold and
               weapons.

HID-TRAP GIF - Down at the end of this corridor, an Orc waites.  Normally,
               he comes to you, but not this time?  Why?  Because there is 
               an invisible trap door in front of him that he is trying to 
               lure you to, thus dropping you an unexpected level!  This nasty
               feature was also in the original Ultima on the Apple II.  
               It seems to me that they removed this in the rewrite though.

Addition File
=============

AKLABETH TXT - The original Applesoft source, for the curious.  Who knows,
               maybe someone will want to make a PC version?

RPG      TXT - A short bit on how I first got interested in RPG's.

Notes:
======
Worst        - Getting attacked deep in the dungeon simultaneously by both
 Nightmare     a Gremlin and a Thief.  Each turn, while the Gremlin is 
  Situation    cutting your food supply in half, the Thief is stealing all of
               your weapons, including the weapon that you are using!

Neat         - Monsters retreating when critically hurt.  Then you have to 
 Feature       chase them to finish them off.  This feature wouldn't reappear
               again until Ultima II.

Badly        - There is no save game option.  If you get fed up, you have to
 Needed        start all over again.
  Feature
