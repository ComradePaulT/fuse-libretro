# Fuse PVM libretro core (Windows x64)

This is a Windows RetroArch core built from `libretro/fuse-libretro` at commit `e997e2b`, with a CRT-sized side tape deck and selected changes from the custom standalone Fuse PVM build. The embedded Fuse engine in this upstream libretro port is **1.6.0**; the standalone PVM build is based on **1.10.0**, so this core is not yet a complete port of every standalone change.

## Install

1. Close RetroArch. Locate its `cores` folder. Make a backup if `fuse_libretro.dll` is already there.
2. Copy the supplied `fuse_libretro.dll` into `cores`.
3. Start RetroArch and choose **Load Content** to open a `.tzx`, `.tap`, `.pzx`, or another Fuse-supported file. Choose the Fuse core. You may need **Load Core** and **Install or Restore a Core** if RetroArch has not indexed the copied DLL.
4. In **Quick Menu → Core Options**, choose the correct Spectrum **Model**. The core defaults to 48K; set 128K for a 128K tape. The model now changes immediately. A `.dsk` or `.ipf` requests the +3 automatically on load. To keep your choice after closing RetroArch, use **Manage Core Options** to save it for the core or current game.

For a PVM, use RetroArch's video output and CRT SwitchRes configuration. With a full border this core reports **320×240** and the active machine's actual frame rate; the frontend chooses the physical video mode. Disable RetroArch bilinear filtering if you want sharp native pixels.

## Loading and tape controls

Tape Auto Load and Tape Fast Load default to **disabled**, and tape sound and matched loading volume default to **enabled**. At the 128K startup menu choose **Tape Loader**, then press Play on the deck. On a 48K machine type `LOAD ""` and press Enter before Play. These defaults preserve loading stripes, real-time tape audio, and custom-loader timing. You can change them in Core Options.

Press **F9** or **gamepad R3** to slide the tape deck in or out from the right. While it is open, use **arrow keys / D-pad** to select a button and **Enter / gamepad A** to activate it. **Esc / gamepad B** closes it. Buttons: Play, Stop, Rewind, Fast Forward (five blocks), Previous block, Next block, POKE, Hide. Rewind and block seeking stop the tape first. The deck shows the current block and animates its reels while playing. RetroArch hotkeys may need remapping if F9 or R3 is already assigned by your frontend configuration.

**POKE** opens a full-screen, Multiface-style entry panel. You can also open it directly with **Ctrl+M**. The Spectrum pauses while it is open. Enter a RAM address from **16384–65535** and a value from **0–255**, then choose **Apply POKE**. Decimal numbers work directly; prefix a hexadecimal number with `$` or `0x`. On a keyboard, type into the selected field, use **Tab** to change fields, **Backspace** to erase, **Enter** to apply, and **Esc** to return. On a controller, use the D-pad and A button to select a field, enter digits on the on-screen keypad, then choose Apply POKE or Return to Game; B also returns. This writes the Spectrum's currently visible RAM, as the standalone deck does. The panel resembles a Multiface but does not activate a Multiface ROM or hardware menu. On a banked 128K machine, POKEs to 49152–65535 affect the bank currently paged into that address range.

The core's ordinary Spectrum keyboard and gamepad mapping works as in upstream Fuse. RetroArch's Quick Menu can map a controller to Cursor, Kempston, or Sinclair joystick according to the game. The deck consumes its navigation keys while open and releases any held game controls.

## Multi-side cassettes and disks

Put the sides in one folder and create a plain-text `.m3u` file beside them, with one filename per line in the order you want. For example, `Game.m3u` can contain:

```text
Game - Side A.tzx
Game - Side B.tzx
```

Load **Game.m3u** in RetroArch. To change sides, open **Quick Menu → Disk Control**, eject the current image, select the next index, then insert it. RetroArch calls these "disks" even when the entries are tapes. After inserting a tape side, use the Datacorder's **Play** button when the game asks for it; changing sides does not restart the Spectrum or start the tape automatically. The deck loads artwork matching each tape side's filename. Single tape files are also exposed to Disk Control so you can append a side there.

The same method works for multi-disk `.dsk` or `.ipf` games; an `.m3u` beginning with either format selects the Spectrum +3 at boot. Keep a playlist's entries as one type of media. Relative paths are resolved from the playlist's folder; use `/` in subfolder paths for portability between Windows and Steam Deck.

## Cassette artwork

The deck can now show a cassette image inside its existing frame. Put your images under RetroArch's **System/BIOS directory**, in `fuse\images\tape`. Use the same filename stem as the tape: for `The Addams Family.tzx`, name the image `The Addams Family.png`. PNG, JPG, JPEG, and BMP are supported. If you are unsure where RetroArch's System/BIOS directory is, check **Settings → Directory → System/BIOS** in RetroArch.

As a second location, the core also checks an `images\tape` folder beside the tape file. For example, if a game is at `D:\Spectrum\The Addams Family.tzx`, it can use `D:\Spectrum\images\tape\The Addams Family.png`. The System/BIOS location takes priority. Artwork is read when content loads; close and reopen the tape in RetroArch after adding or changing an image. If no matching image is found, the built-in Datacorder graphic remains.

## Verification and limits

The DLL built successfully with MinGW-w64 and passed a libretro frontend smoke test at 320×240 with audio. A live switch from 48K to 128K changed the machine and its reported frame rate from 50.080128 to 50.021154 Hz. **The Addams Family.tzx** was played at real-time tape speed in the 128K Tape Loader: the test reached its title screen and later in-game screens after traversing its custom TZX blocks. The tested file itself is not included. This test does not prove every protection scheme or every Spectrum model. A full RetroArch session on the PVM has not yet been performed.

The core includes the Z80 undocumented repeat-instruction flag fix used in the standalone PVM build, a stronger loading-noise mix, exact machine frame rate reporting, a manual tape workflow, the cassette deck, and automatic cassette-image matching. Other 1.10.0 standalone accuracy changes, the box-art browser, and interactive spindle alignment are not yet ported to this 1.6.0-based libretro engine.

This repository contains the modified source and the Steam Deck build script. Fuse and fuse-libretro are licensed under the GPL; see `LICENSE` and `fuse/COPYING`.

Each push to `master` also builds an x86-64 Linux `fuse_libretro.so` in GitHub Actions. Download the `fuse-libretro-steam-deck` artifact from that run to install the updated core on Steam Deck.
