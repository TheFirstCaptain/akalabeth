# Legacy Inventory

## Source And Notes

| Path | Role |
| --- | --- |
| `AKLABETH.TXT` | Working Applesoft BASIC source listing |
| `README.1ST` | Historical description of files, screenshots, monsters, and notes |
| `RPG.TXT` | Historical essay about the author's RPG background |
| `udic.txt` | Ultima Dragons archive note |

## Screenshots

| Path | Subject |
| --- | --- |
| `STARTUP1.GIF` | lucky number and level prompt |
| `STARTUP2.GIF` | character qualities and class selection |
| `STARTUP3.GIF` | Akalabeth welcome/generation screen |
| `OUTSIDE.GIF` | overworld 3x3 local view |
| `TOWN.GIF` | store/status screen |
| `LB-1.GIF` | first Lord British quest |
| `LB-2.GIF` | follow-up Lord British quest |
| `LB-3.GIF` | final Lord British message |
| `CHEST.GIF` | dungeon chest |
| `HID-TRAP.GIF` | hidden trap setup |
| `MNSTR-01.GIF` through `MNSTR-10.GIF` | monster drawings in difficulty order |

## Platform Dependencies

- Applesoft BASIC line-number control flow.
- Apple II text and graphics commands: `TEXT`, `HOME`, `HGR`, `HCOLOR`, `HPLOT`, `VTAB`, `HTAB`, `NORMAL`, and `INVERSE`.
- Apple II keyboard and memory operations: `PEEK`, `POKE`, `CALL`, `PR#`, and `IN#`.
- Applesoft random-number behavior through `RND`, including negative seeding.
- No save-game system is described in `README.1ST`.

## Initial Behavior Areas

| Area | BASIC Lines |
| --- | --- |
| startup and setup | `0-99`, `60000-60075` |
| store/status | `60080-60250` |
| overworld drawing | `100-190` |
| dungeon drawing | `200-490`, `3087-3089` |
| dungeon generation | `500-590` |
| command loop and movement | `1000-1587` |
| attack and magic | `1650-1697` |
| monster placement | `2000-2090` |
| monster turns | `4000-4999` |
| death | `6000-6060` |
| Lord British quests | `7000-7990` |
