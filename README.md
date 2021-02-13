# rkplay - Ron Klaren's BattleSquadron music player

a Reverse-Engineered Ron Klaren's Battlesquadron Amiga music replay.

-------------------------------------------

### Usage

     rkplay [OPTION] <song.rk>

#### Options

| Short|      Long options|                                   Description|
|------|------------------|----------------------------------------------|
| `-h` | `--help --usage` | Print this help message and exit             |
| `-V` | `--version`      | Print version message and exit               |
| `-r` | `--rate=Hz[k]`   | Set sampling rate                            |
| `-m` | `--mute=CHANS`   | Mute selected channels (bit-field or string) |
| `-o` | `--output=URI`   | Set output file name (-w or -c).             |
| `-c` | `--stdout`       | Output raw PCM to stdout or file (host s16)  |
| `-n` | `--null`         | Output to the void                           |
| `-w` | `--wav`          | Generated a .wav file                        |
| `-s` | `--stats`        | Print various statistics on exit             |
