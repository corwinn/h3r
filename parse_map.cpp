/**** BEGIN LICENSE BLOCK ****

BSD 3-Clause License

Copyright (c) 2021-2023, the wind.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**** END LICENCE BLOCK ****/

// This is where it all started. Q'N'D code for FF analysis.
// Actually it started because I grew tired of the original game freezing
// on turn x, and rendering the map not complete-able.

// Its not tested on many maps. It won't be, because there will be a specific
// library doing all the FF parsing when the project reaches certain phase.
// Thus, issues related to Q'N'D code will be closed without wasting time with
// them.
// Why is this being published?
//   - to observe how this project is being developed
//   - create tools based on this one
//   - as showcase of how to not write C++ code :)

// /mnt/workspace/drive64_c/games/h3/Maps/Atlantis_v3.h3m
//c clang++ -std=c++11 -Wall -Wextra -Wshadow -fno-rtti -fno-exceptions -fno-threadsafe-statics -g -O0 parse_map.cpp -o parse_map -Wl,--as-needed -lz

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <zlib.h>

#define EOL "\n"
using byte = unsigned char;

class Stream
{
    Stream * _f {};
public:
    Stream(Stream * s) : _f {s} {};
    virtual ~Stream() {}
    virtual operator bool() { return _f->operator bool(); }
    virtual bool Seek(off_t pos) { return _f->Seek (pos); }
    virtual off_t Tell() { return _f->Tell (); }
    virtual off_t Size() { return _f->Size (); }
    virtual bool Read(void * b, size_t n = 1) { return _f->Read (b, n); }
    virtual bool Write(const void * b, size_t n = 1) { return _f->Write (b, n); }
    // const Stream * BaseStream() const { return _f; }
};

// Caution: this is very simple and optimistic (all is ok) filestream!
class FileStream final: public Stream
{
    FILE * _f {};
    off_t _size_on_open {};
public:
    FileStream(const char * n, const char * mode = "rb")
        : Stream{nullptr}, _f {fopen (n, mode)}
    {
        fseek (_f, 0, SEEK_END), _size_on_open = ftell (_f);
        fseek (_f, 0, SEEK_SET);
    }
    virtual ~FileStream() override { if (_f) fclose (_f), _f = 0; }
    virtual operator bool() override { return 0 != _f; }
    virtual bool Seek(off_t pos) override
    {
        return *this ? 0 == fseek (_f, pos, SEEK_SET) : false;
    }
    virtual off_t Tell() override { return *this ? ftell (_f) : -1; }
    virtual off_t Size() override { return _size_on_open; }
    virtual bool Read(void * b, size_t n = 1) override
    {
        return *this ? ! b ? false : n == fread (b, 1, n, _f) : false;
    }
    virtual bool Write(const void * b, size_t n = 1) override
    {
        return *this ? ! b ? false : n == fwrite (b, 1, n, _f) : false;
    }
};

class zlibInflateStream final: public Stream
{
    z_stream _z {};
    int r {~Z_OK};

    static const int BUF_SIZE {4096};
    byte buf[BUF_SIZE] {};
    off_t _read {};
public:
    zlibInflateStream(Stream * data)
        : Stream {data}, r {inflateInit2 (&_z, 31)} // gzip h3m
    {
        _z.next_in = buf, _z.avail_in = 0;
    }
    virtual ~zlibInflateStream() override { inflateEnd (&_z); }
    virtual operator bool() override { return Z_OK == r; }
    virtual bool Seek(off_t) override { return false; } // can't seek
    virtual off_t Tell() override { return _read; }     // can tell
    virtual off_t Size() override { return -1; }        // no idea about usize
    virtual bool Read(void * b, size_t n = 1) override  // can read
    {
        if (! b || n < 1) return false; // param check
        // printf ("requested %zu bytes" EOL, n);
        _z.next_out = (byte *)b, _z.avail_out = n;
        while (_z.avail_out) {
            if (0 == _z.avail_in) { // block read from the decorated one
                int a = Stream::Size () - Stream::Tell ();
                // printf ("Avail: %d bytes" EOL, a);
                if (a <= 0) return false; // no more input
                int c = a < BUF_SIZE ? a : BUF_SIZE;
                _z.avail_in = c;
                _z.next_in = buf;
                Stream::Read (buf, c);
                // printf ("Read: %d bytes" EOL, c);
            }
            r = inflate (&_z, Z_NO_FLUSH);
            // printf ("r: %d" EOL, r);
            // "be called until it returns Z_STREAM_END or an error"
            if (r != Z_OK && r != Z_STREAM_END) return false;
            // not enough input
            if (Z_STREAM_END == r && _z.avail_out) return false;
        }
        return _read += n, true;
    }
    virtual bool Write(const void *, size_t) override { return false; }
};

template <typename T> class Some0Bytes final
{
    T * _b {};
public:
    Some0Bytes(size_t n)
    {
        if (n < 0 || n > 10000) { // size 0 is allowed in town timed event
            printf ("suspicious byte seqeunce of size %zu" EOL, n);
            exit (1);
        }
        _b = (T *)calloc (n, sizeof(T));
        if (! _b) printf ("Can't alloc %zu bytes. Bye." EOL, n), exit (1);
    }
    ~Some0Bytes() { if (_b) free (_b), _b = nullptr; }
    operator byte *() { return (byte *)_b; }
    operator const byte *() const { return (const byte *)_b; }
    operator char *() { return (char *)_b; }
    operator const char *() const { return (const char *)_b; }
    operator void *() { return _b; }
    T & operator [](int i) { return  _b[i]; }
};

namespace {static struct L final
{
    template <typename T> L & Fmt(const char * f, T & v)
    {
        return printf (f, v), *this;
    }
    L & operator<<(const char * v) { return Fmt ("%s", v); }
    L & operator<<(int v) { return Fmt ("%d", v); }
    L & operator<<(short v) { return Fmt ("%d", v); }
    L & operator<<(byte v) { return Fmt ("%d", v); }
    L & operator<<(L & l) { return l; }
} Log;}

static inline L & print_byte_sequence(const byte * b, size_t n)
{
    if (n <= 0) return Log << "{}";
    auto e = b + n;
    Log.Fmt ("{%002X", *b++);
    while (b < e) Log.Fmt (" %002X", *b++);
    return Log << "}";
}

#define MAP_ROE 0x0e
#define MAP_AB  0x15
#define MAP_SOD 0x1c
#define MAP_WOG 0x33
#define MAP_VERSION(V) (MAP_ROE == V ? "RoE" : \
                        MAP_AB  == V ? "AB"  : \
                        MAP_SOD == V ? "SoD" : \
                        MAP_WOG == V ? "WoG" : "Unknown")
#define MAP_BOOL(V) (V ? "true" : "false")

#define MAP_PLAYERS           8
#define MAP_FACTIONS          9
#define MAP_HEROES            156
#define MAP_PRIMARY_SKILLS    4
#define MAP_RESOURCE_QUANTITY 8
#define MAP_NO_PATROL_RADIUS  255
#define MAP_SKILL_QUANTITY    28

#define MAP_AI_RANDOM 0
#define MAP_AI_WARRIOR 1
#define MAP_AI_BUILDER 2
#define MAP_AI_EXPLORER 3
#define MAP_AI(V) (MAP_AI_RANDOM   == V ? "Random"   : \
                   MAP_AI_WARRIOR  == V ? "Warrior"  : \
                   MAP_AI_BUILDER  == V ? "Builder"  : \
                   MAP_AI_EXPLORER == V ? "Explorer" : "Unknown")

//TODO guess
#define MAP_DEFAULT_HERO_ID 0xff
#define MAP_DEFAULT_HERO_PORTRAIT_ID 0xff

#define MAP_VICTORY_CONDITION_ARTIFACT        0
#define MAP_VICTORY_CONDITION_GATHERTROOP     1
#define MAP_VICTORY_CONDITION_GATHERRESOURCE  2
#define MAP_VICTORY_CONDITION_UPG_CITY        3
#define MAP_VICTORY_CONDITION_BUILDGRAIL      4
#define MAP_VICTORY_CONDITION_BEATHERO        5
#define MAP_VICTORY_CONDITION_CAPTURECITY     6
#define MAP_VICTORY_CONDITION_BEATMONSTER     7
#define MAP_VICTORY_CONDITION_TAKEDWELLINGS   8
#define MAP_VICTORY_CONDITION_TAKEMINES       9
#define MAP_VICTORY_CONDITION_TRANSPORTITEM  10
#define MAP_VICTORY_CONDITION_STD           255
#define MAP_VC(V) (MAP_VICTORY_CONDITION_ARTIFACT        == V ? "Artifact"     : \
                   MAP_VICTORY_CONDITION_GATHERTROOP     == V ? "Troop"        : \
                   MAP_VICTORY_CONDITION_GATHERRESOURCE  == V ? "Resource"     : \
                   MAP_VICTORY_CONDITION_UPG_CITY        == V ? "Upgrade City" : \
                   MAP_VICTORY_CONDITION_BUILDGRAIL      == V ? "Grail"        : \
                   MAP_VICTORY_CONDITION_BEATHERO        == V ? "Defeat Hero"  : \
                   MAP_VICTORY_CONDITION_CAPTURECITY     == V ? "Capture City" : \
                   MAP_VICTORY_CONDITION_BEATMONSTER     == V ? "Defeat Mon"   : \
                   MAP_VICTORY_CONDITION_TAKEDWELLINGS   == V ? "Dwellings"    : \
                   MAP_VICTORY_CONDITION_TAKEMINES       == V ? "Mines"        : \
                   MAP_VICTORY_CONDITION_TRANSPORTITEM   == V ? "Transport"    : \
                   MAP_VICTORY_CONDITION_STD             == V ? "Standard"     : \
                   "Unknown")

#define MAP_LOSS_CONDITION_CASTLE        0
#define MAP_LOSS_CONDITION_HERO          1
#define MAP_LOSS_CONDITION_TIMEEXPIRES   2
#define MAP_LOSS_CONDITION_STD         255
#define MAP_LC(V) (MAP_LOSS_CONDITION_CASTLE      == V ? "Castle"   : \
                   MAP_LOSS_CONDITION_HERO        == V ? "Hero"     : \
                   MAP_LOSS_CONDITION_TIMEEXPIRES == V ? "Time"     : \
                   MAP_LOSS_CONDITION_STD         == V ? "Standard" : \
                   "Unknown")

#define MAP_TOWN    0
#define MAP_CITY    1
#define MAP_CAPITOL 2
#define MAP_HALL_LEVEL(V) (MAP_TOWN    == V ? "Town"    : \
                           MAP_CITY    == V ? "City"    : \
                           MAP_CAPITOL == V ? "Capitol" : "Unknown")
#define MAP_FORT    0
#define MAP_CITADEL 1
#define MAP_CASTLE  2
#define MAP_CASTLE_LEVEL(V) (MAP_FORT    == V ? "Fort"    : \
                             MAP_CITADEL == V ? "Citadel" : \
                             MAP_CASTLE  == V ? "Castle"  : "Unknown")

#define MAP_HERO_MALE   0
#define MAP_HERO_FEMALE 1
#define MAP_HERO_SEX(V) (MAP_HERO_MALE   == V ? "Male"   : \
                         MAP_HERO_FEMALE == V ? "Female" : "Unknown")

#define MAP_TERRAIN_DIRT         0
#define MAP_TERRAIN_SAND         1
#define MAP_TERRAIN_GRASS        2
#define MAP_TERRAIN_SNOW         3
#define MAP_TERRAIN_SWAMP        4
#define MAP_TERRAIN_ROUGH        5
#define MAP_TERRAIN_SUBTERRANEAN 6
#define MAP_TERRAIN_LAVA         7
#define MAP_TERRAIN_WATER        8
#define MAP_TERRAIN_ROCK         9
#define MAP_TT(V) (MAP_TERRAIN_DIRT         == V ? "Dirt"  : \
                   MAP_TERRAIN_SAND         == V ? "Sand"  : \
                   MAP_TERRAIN_GRASS        == V ? "Grass" : \
                   MAP_TERRAIN_SNOW         == V ? "Snow"  : \
                   MAP_TERRAIN_SWAMP        == V ? "Swamp" : \
                   MAP_TERRAIN_ROUGH        == V ? "Rough" : \
                   MAP_TERRAIN_SUBTERRANEAN == V ? "Subt"  : \
                   MAP_TERRAIN_LAVA         == V ? "Lava"  : \
                   MAP_TERRAIN_WATER        == V ? "Water" : \
                   MAP_TERRAIN_ROCK         == V ? "Rock"  : "Unknown")

#define MAP_NO_RIVER    0
#define MAP_CLEAR_RIVER 1
#define MAP_ICY_RIVER   2
#define MAP_MUDDY_RIVER 3
#define MAP_LAVA_RIVER  4
#define MAP_RIVER(V) (MAP_NO_RIVER    == V ? "None"  : \
                      MAP_CLEAR_RIVER == V ? "Clear" : \
                      MAP_ICY_RIVER   == V ? "Icy"   : \
                      MAP_MUDDY_RIVER == V ? "Muddy" : \
                      MAP_LAVA_RIVER  == V ? "Lava"  : "Unknown")

#define MAP_NO_ROAD          0
#define MAP_DIRT_ROAD        1
#define MAP_GRAVEL_ROAD      2
#define MAP_COBBLESTONE_ROAD 3
#define MAP_ROAD(V) (MAP_NO_ROAD          == V ? "None"        : \
                     MAP_DIRT_ROAD        == V ? "Dirt"        : \
                     MAP_GRAVEL_ROAD      == V ? "Gravel"      : \
                     MAP_COBBLESTONE_ROAD == V ? "Cobblestone" : "Unknown")


#define MAP_O_ALTAR_OF_SACRIFICE           2
#define MAP_O_ANCHOR_POINT                 3
#define MAP_O_ARENA                        4
#define MAP_O_ARTIFACT                     5
#define MAP_O_PANDORAS_BOX                 6
#define MAP_O_BLACK_MARKET                 7
#define MAP_O_BOAT                         8
#define MAP_O_BORDERGUARD                  9
#define MAP_O_KEYMASTER                   10
#define MAP_O_BUOY                        11
#define MAP_O_CAMPFIRE                    12
#define MAP_O_CARTOGRAPHER                13
#define MAP_O_SWAN_POND                   14
#define MAP_O_COVER_OF_DARKNESS           15
#define MAP_O_CREATURE_BANK               16
#define MAP_O_CREATURE_GENERATOR1         17
#define MAP_O_CREATURE_GENERATOR2         18
#define MAP_O_CREATURE_GENERATOR3         19
#define MAP_O_CREATURE_GENERATOR4         20
#define MAP_O_CURSED_GROUND1              21
#define MAP_O_CORPSE                      22
#define MAP_O_MARLETTO_TOWER              23
#define MAP_O_DERELICT_SHIP               24
#define MAP_O_DRAGON_UTOPIA               25
#define MAP_O_EVENT                       26
#define MAP_O_EYE_OF_MAGI                 27
#define MAP_O_FAERIE_RING                 28
#define MAP_O_FLOTSAM                     29
#define MAP_O_FOUNTAIN_OF_FORTUNE         30
#define MAP_O_FOUNTAIN_OF_YOUTH           31
#define MAP_O_GARDEN_OF_REVELATION        32
#define MAP_O_GARRISON                    33
#define MAP_O_HERO                        34
#define MAP_O_HILL_FORT                   35
#define MAP_O_GRAIL                       36
#define MAP_O_HUT_OF_MAGI                 37
#define MAP_O_IDOL_OF_FORTUNE             38
#define MAP_O_LEAN_TO                     39
#define MAP_O_LIBRARY_OF_ENLIGHTENMENT    41
#define MAP_O_LIGHTHOUSE                  42
#define MAP_O_MONOLITH1                   43
#define MAP_O_MONOLITH2                   44
#define MAP_O_MONOLITH3                   45
#define MAP_O_MAGIC_PLAINS1               46
#define MAP_O_SCHOOL_OF_MAGIC             47
#define MAP_O_MAGIC_SPRING                48
#define MAP_O_MAGIC_WELL                  49
#define MAP_O_MERCENARY_CAMP              51
#define MAP_O_MERMAID                     52
#define MAP_O_MINE                        53
#define MAP_O_MONSTER                     54
#define MAP_O_MYSTICAL_GARDEN             55
#define MAP_O_OASIS                       56
#define MAP_O_OBELISK                     57
#define MAP_O_REDWOOD_OBSERVATORY         58
#define MAP_O_OCEAN_BOTTLE                59
#define MAP_O_PILLAR_OF_FIRE              60
#define MAP_O_STAR_AXIS                   61
#define MAP_O_PRISON                      62
#define MAP_O_PYRAMID                     63
#define MAP_O_WOG_OBJECT                  63
#define MAP_O_RALLY_FLAG                  64
#define MAP_O_RANDOM_ART                  65
#define MAP_O_RANDOM_TREASURE_ART         66
#define MAP_O_RANDOM_MINOR_ART            67
#define MAP_O_RANDOM_MAJOR_ART            68
#define MAP_O_RANDOM_RELIC_ART            69
#define MAP_O_RANDOM_HERO                 70
#define MAP_O_RANDOM_MONSTER              71
#define MAP_O_RANDOM_MONSTER_L1           72
#define MAP_O_RANDOM_MONSTER_L2           73
#define MAP_O_RANDOM_MONSTER_L3           74
#define MAP_O_RANDOM_MONSTER_L4           75
#define MAP_O_RANDOM_RESOURCE             76
#define MAP_O_RANDOM_TOWN                 77
#define MAP_O_REFUGEE_CAMP                78
#define MAP_O_RESOURCE                    79
#define MAP_O_SANCTUARY                   80
#define MAP_O_SCHOLAR                     81
#define MAP_O_SEA_CHEST                   82
#define MAP_O_SEER_HUT                    83
#define MAP_O_CRYPT                       84
#define MAP_O_SHIPWRECK                   85
#define MAP_O_SHIPWRECK_SURVIVOR          86
#define MAP_O_SHIPYARD                    87
#define MAP_O_SHRINE_OF_MAGIC_INCANTATION 88
#define MAP_O_SHRINE_OF_MAGIC_GESTURE     89
#define MAP_O_SHRINE_OF_MAGIC_THOUGHT     90
#define MAP_O_SIGN                        91
#define MAP_O_SIRENS                      92
#define MAP_O_SPELL_SCROLL                93
#define MAP_O_STABLES                     94
#define MAP_O_TAVERN                      95
#define MAP_O_TEMPLE                      96
#define MAP_O_DEN_OF_THIEVES              97
#define MAP_O_TOWN                        98
#define MAP_O_TRADING_POST                99
#define MAP_O_LEARNING_STONE             100
#define MAP_O_TREASURE_CHEST             101
#define MAP_O_TREE_OF_KNOWLEDGE          102
#define MAP_O_SUBTERRANEAN_GATE          103
#define MAP_O_UNIVERSITY                 104
#define MAP_O_WAGON                      105
#define MAP_O_WAR_MACHINE_FACTORY        106
#define MAP_O_SCHOOL_OF_WAR              107
#define MAP_O_WARRIORS_TOMB              108
#define MAP_O_WATER_WHEEL                109
#define MAP_O_WATERING_HOLE              110
#define MAP_O_WHIRLPOOL                  111
#define MAP_O_WINDMILL                   112
#define MAP_O_WITCH_HUT                  113
#define MAP_O_HOLE                       124
#define MAP_O_RANDOM_MONSTER_L5          162
#define MAP_O_RANDOM_MONSTER_L6          163
#define MAP_O_RANDOM_MONSTER_L7          164
#define MAP_O_BORDER_GATE                212
#define MAP_O_FREELANCERS_GUILD          213
#define MAP_O_HERO_PLACEHOLDER           214
#define MAP_O_QUEST_GUARD                215
#define MAP_O_RANDOM_DWELLING            216
#define MAP_O_RANDOM_DWELLING_LVL        217
#define MAP_O_RANDOM_DWELLING_FACTION    218
#define MAP_O_GARRISON2                  219
#define MAP_O_ABANDONED_MINE             220
#define MAP_O_TRADING_POST_SNOW          221
#define MAP_O_CLOVER_FIELD               222
#define MAP_O_CURSED_GROUND2             223
#define MAP_O_EVIL_FOG                   224
#define MAP_O_FAVORABLE_WINDS            225
#define MAP_O_FIERY_FIELDS               226
#define MAP_O_HOLY_GROUNDS               227
#define MAP_O_LUCID_POOLS                228
#define MAP_O_MAGIC_CLOUDS               229
#define MAP_O_MAGIC_PLAINS2              230
#define MAP_O_ROCKLANDS                  231

#define MAP_SEER_NOTHING          0
#define MAP_SEER_EXPERIENCE       1
#define MAP_SEER_MANA_POINTS      2
#define MAP_SEER_MORALE_BONUS     3
#define MAP_SEER_LUCK_BONUS       4
#define MAP_SEER_RESOURCES        5
#define MAP_SEER_PRIMARY_SKILL    6
#define MAP_SEER_SECONDARY_SKILL  7
#define MAP_SEER_ARTIFACT         8
#define MAP_SEER_SPELL            9
#define MAP_SEER_CREATURE        10
#define MAP_SEER_REWARD(V) (MAP_SEER_NOTHING         == V ? "None"  : \
                            MAP_SEER_EXPERIENCE      == V ? "Exp"    : \
                            MAP_SEER_MANA_POINTS     == V ? "Mana"   : \
                            MAP_SEER_MORALE_BONUS    == V ? "Morale" : \
                            MAP_SEER_LUCK_BONUS      == V ? "Luck"   : \
                            MAP_SEER_RESOURCES       == V ? "Res"    : \
                            MAP_SEER_PRIMARY_SKILL   == V ? "PSkill" : \
                            MAP_SEER_SECONDARY_SKILL == V ? "SSkill" : \
                            MAP_SEER_ARTIFACT        == V ? "Art"    : \
                            MAP_SEER_SPELL           == V ? "Spell"  : \
                            MAP_SEER_CREATURE        == V ? "Cre"  : \
                            "Unknown")

/* map editor (SoD) limits:
 "There is a maximum of 32 objects of type "Learning Stone" on any map!"
 "There is a maximum of 32 objects of type "Star Axis" on any map!"
 "There is a maximum of 144 objects of type "Creature Generator 1" on any map!"
*/

template <typename T> T Read(Stream & br)
{
    T data;
    if (! br.Read (&data, sizeof(T))) { printf ("Read failed\n"); exit (1); }
    return data;
}

// Avoid manual size computation. "virtual template <typename T>":
template <typename T> bool Read(Stream & br, T & d, size_t n = 1)
{
    if (! br.Read (&d, n * sizeof(T))) { printf ("Read failed\n"); exit (1); }
    return true;
}
template <typename T> void Read(Stream & br, T * d, size_t n = 1)
{
    if (! br.Read (d, n * sizeof(T))) { printf ("Read failed\n"); exit (1); }
}

static bool ReadSkip(Stream & br, size_t n)
{
    Some0Bytes<byte> buf (n);
    if (! br.Read (buf, n)) {
        printf ("Read failed\n");
        exit (1);
    }
    return true;
}

struct MapString
{
    int Len;
    Some0Bytes<byte> Chars;
    MapString(Stream & s)
        : Len {Read<int> (s)}, Chars {static_cast<size_t>(Len+1)}
    {
        Read (s, Chars.operator byte * (), Len);
    }
};
#pragma pack(push, 1)
struct MapObject final
{
    MapString SpriteName;
    byte PassMask[6]; // w,h = (sprite_name.s(.def/.msk)).bytes[0],[1]
    byte TrigMask[6]; // the mask max is 8x6 (at the editor there is a lake 7x3)
    byte Unk1[2];
    short TerrainMask; // ?
    int EdId; // objects[i] = def_id; string ref - editor name
    int SubId;
    byte Type; // no idea yet
    byte PrintPriority; // ?
    byte Unk2[16];
    MapObject(Stream & s)
        : SpriteName {s}
    {
        s.Read (&PassMask, sizeof(MapObject) - sizeof(MapString));
    }
};
#pragma pack(pop)
L & operator<<(L &, MapObject & obj)
{
    Log << " sprite: "
        << obj.SpriteName.Chars.operator const char * () << EOL
        << " PMask: " << print_byte_sequence (obj.PassMask, 6)
        << ", TMask: " << print_byte_sequence (obj.TrigMask, 6) << EOL
        << " U1: " << print_byte_sequence (obj.Unk1, 2)
        << ", TerrMask: " << obj.TerrainMask
        << ", EdtorId: " << Log.Fmt ("%3d", obj.EdId)
        << ", SubId: " << Log.Fmt ("%3d", obj.SubId)
        << ", Type: " << obj.Type
        << ", zOrder: " << obj.PrintPriority << EOL
        << " U2: " << print_byte_sequence (obj.Unk2, 16) << EOL;
    return Log;
}

static bool read_string(Stream & br, const char * what)
{
    int len;
    if (! Read (br, len)) return false;
    if (len < 0 || len > 10000) // size 0 is allowed in town timed event
        //TODO get the decompressed offset via BaseStream interface
        return printf ("suspicious string of size %d" EOL, len), false;
    Some0Bytes<byte> str (len + 1); // add my '\0' for printf
    if (len > 0 && ! br.Read (str, len))
        return printf ("failed to read string of size %d" EOL, len), false;
    printf ("%s%s" EOL, what, (const char *)str);
    return true;
}

static bool read_pos(Stream & br)
{
    byte x, y, z;
    bool result = Read (br, x) && Read (br, y) && Read (br, z);
    if (result) printf ("x, y, z = %d, %d, %d", x, y, z);
    return result;
}

static bool read_id_ROE8_16(Stream & br, int map_version, short int & id)
{
    if (MAP_ROE == map_version) {
        byte id8;
        if (! Read (br, id8)) return false;
        id = id8;
    } else
        if (! Read (br, id)) return false;
    return true;
}

static bool loadArtifactToSlot(Stream & br, int map_version)
{
    short int art_id;
    read_id_ROE8_16 (br, map_version, art_id);
    printf ("id: %d" EOL, art_id);
    return true;
}

static bool readResourses(Stream & br)
{
    for(int i = 0; i < MAP_RESOURCE_QUANTITY-1; i++)
    {
        int res;
        if (! Read (br, res)) return false;
        printf ("  Resource%d: %d" EOL, i, res);
    }
    return true;
}

static bool readCreatureSet(Stream & br, int map_version, int num)
{
    for (int i = 0; i < num; i++) {
        short int id, cnum;
        read_id_ROE8_16 (br, map_version, id);
        printf ("  Cre%d: id: %d" EOL, i, id);
        if (! Read (br, cnum)) return false;
        printf ("  Cre%d: num: %d" EOL, i, cnum);
    }
    return true;
}

static bool loadArtifactsOfHero(Stream & br, int map_version, int i)
{
    byte has_arts;
    if (! Read (br, has_arts)) return false;
    printf ("Customized Hero #%3d: has arts: %s" EOL, i,
        MAP_BOOL(has_arts));
    if (has_arts) {
        for (int a = 0; a < 16; a++) { // TODO 16 ?
            printf ("Customized Hero #%3d: art%d", i, a);
            if (! loadArtifactToSlot (br, map_version)) return false;
            printf (EOL);
        }
        if (map_version > MAP_AB) {
            printf ("Customized Hero #%3d: art_mach4", i);
            if (! loadArtifactToSlot (br, map_version)) return false;
        }
        printf ("Customized Hero #%3d: art_spellbook", i);
        if (! loadArtifactToSlot (br, map_version)) return false;
        printf (EOL);
        if (map_version > MAP_ROE) {
            printf ("Customized Hero #%3d: art_misc5", i);
            if (! loadArtifactToSlot (br, map_version)) return false;
            printf (EOL);
        } else if (! ReadSkip (br, 1)) return 1;
        short int bag_quantity;
        if (! Read (br, bag_quantity)) return false;
        printf ("Customized Hero #%3d: bag arts: %d" EOL, i,
            bag_quantity);
        for (int b = 0; b < bag_quantity; b++) {
            printf ("Customized Hero #%3d: art_bag%d", i, b);
            if (! loadArtifactToSlot (br, map_version)) return false;
            printf (EOL);
        }
    } // if (has_arts)
    return true;
}

static bool readMessageAndGuards(Stream & br, int map_version)
{
    bool has_message;
    if (! Read (br, has_message)) return false;
    printf (" MsgGuard: has_message: %s" EOL, MAP_BOOL(has_message));
    if (! has_message) return true;
    read_string (br, " MsgGuard: Msg: ");
    bool has_guard;
    if (! Read (br, has_guard)) return false;
    printf (" MsgGuard: has_guard: %s" EOL, MAP_BOOL(has_guard));
    if (has_guard)
        if (! readCreatureSet (br, map_version, 7)) return false;
    if (! ReadSkip (br, 4)) return false;
    return true;
}

static bool readQuest(Stream & br, byte & mission_type)
{
    if (! Read (br, mission_type)) return false;
    printf (" Quest: mission_type: %d" EOL, mission_type);
    switch (mission_type) {
        case 0: return printf (" Quest: mtype 0?" EOL), false;
        case 2: {
            for (int i = 0; i < 4; i++) {
                byte m2stats;
                if (! Read (br, m2stats)) return false;
                printf (" Quest: m2stats%d: %d" EOL, i, m2stats);
             }
        } break;
        case 1:
        case 3:
        case 4: {
            int m13489val; //TODO WT nice mountain view?
            if (! Read (br, m13489val)) return false;
            printf (" Quest: m13489val?: %d" EOL, m13489val);
        } break;
        case 5: {
            byte art_num;
            if (! Read (br, art_num)) return false;
            printf (" Quest: arts: %d" EOL, art_num);
            for (int i = 0; i < art_num; i++) {
                short int art_id;
                if (! Read (br, art_id)) return false;
                printf (" Quest: art%d: %d" EOL, i, art_id);
            }
        } break;
        case 6: {
            byte cre_num;
            if (! Read (br, cre_num)) return false;
            printf (" Quest: cre: %d" EOL, cre_num);
            for (int i = 0; i < cre_num; i++) {
                short int cre_type, cre_qty;
                if (! Read (br, cre_type)) return false;
                if (! Read (br, cre_qty)) return false;
                printf (" Quest: cre%d: type: %d, qty: %d" EOL, i, cre_type,
                    cre_qty);
            }
        } break;
        case 7: {
            for (int i = 0; i < 7; i++) {
                int res_qty;
                if (! Read (br, res_qty)) return false;
                printf (" Quest: res%d: qty: %d" EOL, i, res_qty);
            }
        } break;
        case 8:
        case 9: {
            byte m13489val; //TODO WT nice mountain view?
            if (! Read (br, m13489val)) return false;
            printf (" Quest: m13489val?: %d" EOL, m13489val);
        } break;
    }; // switch (mission_type)
    int limit;
    if (! Read (br, limit)) return false;
    printf (" Quest: limit: %d" EOL, limit);
    read_string (br, " Quest: 1st visit: ");
    read_string (br, " Quest: next visit: ");
    read_string (br, " Quest: completed: ");
    return true;
}

#define TELL_POS printf ("Unc Stream Pos: %.8lX" EOL, br.Tell ());

int main(int argc, char ** argv)
{
    if (2 != argc) return printf ("parse_map foo.h3m" EOL), 1;

    FileStream cdata {argv[1]};
    zlibInflateStream br {&cdata};

    // header
    int map_version;
    if (! Read (br, map_version)) return 1;
    printf ("Map Version  : %s" EOL, MAP_VERSION(map_version));
    byte areAnyPlayers;
    if (! Read (br, areAnyPlayers)) return 1;
    printf ("areAnyPlayers: %s" EOL, MAP_BOOL(areAnyPlayers));
    int h, w;
    if (! Read (br, w)) return 1;
    h = w;
    printf ("Size         : %d x %d" EOL, w, h);
    byte two_level;
    if (! Read (br, two_level)) return 1;
    printf ("Levels       : %d" EOL, two_level ? 2 : 1);
    if (! read_string(br, "Name         : ")) return 1;
    if (! read_string(br, "Description  : ")) return 1;
    byte difficulty;
    if (! Read (br, difficulty)) return 1;
    printf ("Difficulty   : %d" EOL, difficulty);
    byte levelLimit = 0; // avaialble at !RoE
    if (map_version != MAP_ROE && ! Read (br, levelLimit)) return 1;
    printf ("Level Limit  : %d" EOL, levelLimit);
    // readPlayerInfo();
    int map_players {};
    for (int i = 0; i < MAP_PLAYERS; i++) {
        byte canHumanPlay, canComputerPlay;
        if (! Read (br, canHumanPlay)) return 1;
        if (! Read (br, canComputerPlay)) return 1;
        printf ("  Player%d: Human   :%s" EOL, i+1, MAP_BOOL(canHumanPlay));
        printf ("  Player%d: Computer:%s" EOL, i+1, MAP_BOOL(canComputerPlay));
        if (0 == canHumanPlay && 0 == canComputerPlay) // can't play
            switch (map_version) {
                case MAP_SOD:
                case MAP_WOG: if (! ReadSkip (br, 13)) return 1; break;
                case MAP_AB : if (! ReadSkip (br, 12)) return 1; break;
                case MAP_ROE: if (! ReadSkip (br,  6)) return 1; break;
                default: printf ("You have a bug" EOL); return 1;
            }
        else { // can play
            map_players++;
            byte aiTactic;
            if (! Read (br, aiTactic)) return 1;
            printf ("  Player%d: AI   :%s" EOL, i+1, MAP_AI(aiTactic));
            if (MAP_SOD == map_version || MAP_WOG == map_version) {
                byte p7 {}; // unknown
                if (! Read (br, p7)) return 1;
            }
            byte allowedFactions, totalFactions {MAP_FACTIONS};
            if (! Read (br, allowedFactions)) return 1;
            printf ("  Player%d: factions0   :%d" EOL, i+1, allowedFactions);
            if (map_version != MAP_ROE) // its 16-bit
                { if (! Read (br, allowedFactions)) return 1; }
            else totalFactions--; // no Conflux in RoE
            printf ("  Player%d: factions1   :%d" EOL, i+1, allowedFactions);
            byte isFactionRandom;
            if (! Read (br, isFactionRandom)) return 1;
            printf ("  Player%d: random faction :%s" EOL, i+1,
                MAP_BOOL(isFactionRandom));
            byte hasMainTown;
            if (! Read (br, hasMainTown)) return 1;
            printf ("  Player%d: has main town :%s" EOL, i+1,
                MAP_BOOL(hasMainTown));
            if (hasMainTown) {
                byte generateHeroAtMainTown {1}, generateHero {0};
                if (map_version != MAP_ROE) {
                    if (! Read (br, generateHeroAtMainTown)) return 1;
                    if (! Read (br, generateHero)) return 1;
                    printf ("  Player%d: gen at main town :%s" EOL, i+1,
                        MAP_BOOL(generateHeroAtMainTown));
                    printf ("  Player%d: gen hero :%s" EOL, i+1,
                        MAP_BOOL(generateHero));
                }
                printf ("  Player%d: main town pos:", i+1);
                if (! read_pos (br)) return 1;
                printf (EOL);
            }
            byte hasRandomHero, mainCustomHeroId {MAP_DEFAULT_HERO_ID};
            if (! Read (br, hasRandomHero)) return 1;
            printf ("  Player%d: has random hero :%s" EOL, i+1,
                MAP_BOOL(hasRandomHero));
            if (! Read (br, mainCustomHeroId)) return 1;
            printf ("  Player%d: mainCustomHeroId :%d" EOL, i+1, mainCustomHeroId);
            if (mainCustomHeroId != MAP_DEFAULT_HERO_ID) {
                byte mainCustomHeroPortrait {MAP_DEFAULT_HERO_PORTRAIT_ID};
                if (! Read (br, mainCustomHeroPortrait)) return 1;
                printf ("  Player%d: mainCustomHeroPortraitId :%d" EOL, i+1,
                    mainCustomHeroPortrait);
                printf ("  Player%d:", i+1);
                if (! read_string(br, " mainCustomHeroName  : ")) return 1;
            }
            if (map_version != MAP_ROE) {
                byte powerPlaceholders;
                if (! Read (br, powerPlaceholders)) return 1;
                printf ("  Player%d: powerPlaceholders :%d" EOL, i+1,
                    powerPlaceholders);
                byte heroCount;
                if (! Read (br, heroCount)) return 1;
                printf ("  Player%d: heroCount :%d" EOL, i+1, heroCount);
                if (! ReadSkip (br, 3)) return 1; //TODO its probably an int32
                for (int j = 0; j < heroCount; j++) {
                    byte heroId;
                    if (! Read (br, heroId)) return 1;
                    printf ("  Player%d: hero%d Id :%d" EOL, i+1, j+1, heroId);
                    printf ("  Player%d: hero%d", i+1, j+1);
                    if (! read_string(br, " Name  : ")) return 1;
                }
            }
        } // can play
    } // for (int i = 0; i < MAP_PLAYERS; i++)

    // readVictoryLossConditions();
    byte vicCondition;
    if (! Read (br, vicCondition)) return 1;
    printf ("Victory Cond : %s" EOL, MAP_VC(vicCondition));
    if (vicCondition != MAP_VICTORY_CONDITION_STD) {
        byte allowNormalVictory, appliesToAI;
        if (! Read (br, allowNormalVictory)) return 1;
        printf ("  Allow Normal Victory : %s" EOL, MAP_BOOL(allowNormalVictory));
        if (! Read (br, appliesToAI)) return 1;
        printf ("  Applies to AI : %s" EOL, MAP_BOOL(appliesToAI));
        if (map_players <= 1 && allowNormalVictory)
            return printf ("Defect map" EOL), 1;
        switch (vicCondition) {
            case MAP_VICTORY_CONDITION_ARTIFACT: {
                byte objectType;
                if (! Read (br, objectType)) return 1;
                //TODO int16 perhaps
                if (map_version != MAP_ROE)
                    if (! ReadSkip (br,  1)) return 1;
                printf ("  Artifact : %d" EOL, objectType);
            } break;
            case MAP_VICTORY_CONDITION_GATHERTROOP: {
                byte objectType;
                if (! Read (br, objectType)) return 1;
                //TODO int16 perhaps
                if (map_version != MAP_ROE)
                    if (! ReadSkip (br,  1)) return 1;
                printf ("  Troop : %d" EOL, objectType);
                int quantity;
                if (! Read (br, quantity)) return 1;
                printf ("  Troop quantity : %d" EOL, quantity);
            } break;
            case MAP_VICTORY_CONDITION_GATHERRESOURCE: {
                byte objectType;
                if (! Read (br, objectType)) return 1;
                //TODO int16 perhaps
                if (map_version != MAP_ROE)
                    if (! ReadSkip (br,  1)) return 1;
                printf ("  Resource : %d" EOL, objectType);
                int quantity;
                if (! Read (br, quantity)) return 1;
                printf ("  Resource quantity : %d" EOL, quantity);
            } break;
            case MAP_VICTORY_CONDITION_UPG_CITY: {
                printf ("  City pos :");
                if (! read_pos (br)) return 1;
                printf (EOL);
                byte hallLevel, castleLevel;
                if (! Read (br, hallLevel)) return 1;
                printf ("  Hall Level :%s" EOL, MAP_HALL_LEVEL(hallLevel));
                if (! Read (br, castleLevel)) return 1;
                printf ("  Castle Level :%s" EOL, MAP_CASTLE_LEVEL(castleLevel));
            } break;
            case MAP_VICTORY_CONDITION_BUILDGRAIL:
            case MAP_VICTORY_CONDITION_BEATHERO:
            case MAP_VICTORY_CONDITION_CAPTURECITY:
            case MAP_VICTORY_CONDITION_BEATMONSTER: {
                printf ("  Target pos :");
                if (! read_pos (br)) return 1;
                printf (EOL);
            } break;
            case MAP_VICTORY_CONDITION_TAKEDWELLINGS: break;
            case MAP_VICTORY_CONDITION_TAKEMINES: break;
            case MAP_VICTORY_CONDITION_TRANSPORTITEM: {
                byte objectType;
                if (! Read (br, objectType)) return 1;
                printf ("  Item Id : %d" EOL, objectType);
                printf ("  Item pos :");
                if (! read_pos (br)) return 1;
                printf (EOL);
            } break;
            default: return printf ("  Unknown victory condition" EOL), 1;
        } // switch (vicCondition) {
    } // vicCondition != MAP_VICTORY_CONDITION_STD
    byte lossCondition;
    if (! Read (br, lossCondition)) return 1;
    printf ("Loss Cond : %s" EOL, MAP_LC(lossCondition));
    if (lossCondition != MAP_LOSS_CONDITION_STD)
            switch (lossCondition) {
            case MAP_LOSS_CONDITION_CASTLE:
            case MAP_LOSS_CONDITION_HERO: {
                printf ("  Target pos :");
                if (! read_pos (br)) return 1;
                printf (EOL);
            } break;
            case MAP_LOSS_CONDITION_TIMEEXPIRES: {
                short int quantity;
                if (! Read (br, quantity)) return 1;
                printf ("  Time quantity : %d" EOL, quantity);
            } break;
            default: return printf ("  Unknown loss condition" EOL), 1;
        }
    // readTeamInfo();
    byte teams;
    if (! Read (br, teams)) return 1;
    printf ("Teams        : %d" EOL, teams);
    for (int i = 0; teams > 0 && i < MAP_PLAYERS; i++) {
        byte team;
        if (! Read (br, team)) return 1;
        printf ("team[%d]=%d" EOL, i, team);
    }
    // readAllowedHeroes();
    int ma_bytes = MAP_ROE == map_version ? 16 : 20;
    Some0Bytes<byte> map_allowed_heroes (ma_bytes);
    if (! br.Read (map_allowed_heroes, ma_bytes)) return 1;
    printf ("AllowedHeroes: %d bytes" EOL, ma_bytes);
    if (map_version > MAP_ROE) {
        int placeholdersQty;
        if (! Read (br, placeholdersQty)) return 1;
        printf ("placeholdersQty: %d" EOL, placeholdersQty); // whats this?
        for (int i = 0; i < placeholdersQty; i++) {
            byte placeholder;
            if (! Read (br, placeholder)) return 1;
            printf ("placeholders%d: %d" EOL, i, placeholder); // whats this?
        }
    }

    // header ends here according to VCMI

    // readDisposedHeroes();
    if (map_version >= MAP_SOD) {
        byte num_dh;
        if (! Read (br, num_dh)) return 1;
        printf ("Num disposed? heroes : %d" EOL, num_dh);
        for (int i = 0; i < num_dh; i++) {
            byte id, portrait_id, players;
            if (! Read (br, id)) return 1;
            printf ("  disposed hero%d Id: %d" EOL, i, id);
            if (! Read (br, portrait_id)) return 1;
            printf ("  disposed hero%d Portrait Id: %d" EOL, i, portrait_id);
            printf ("  disposed hero%d", i);
            if (! read_string(br, " Name: ")) return 1;
            if (! Read (br, players)) return 1;
            printf ("  disposed hero%d Players?: %d" EOL, i, players);
        }
    }
    if (! ReadSkip (br, 31)) return 1;
    // readAllowedArtifacts();
    if (map_version != MAP_ROE) {
        int bytes2 = MAP_AB == map_version ? 17 : 18;
        Some0Bytes<byte> map_allowed_artifacts (bytes2);
        if (! br.Read (map_allowed_artifacts, bytes2)) return 1;
        printf ("AllowedArtifacts: %d bytes" EOL, bytes2);
    }
    // readAllowedSpellsAbilities();
    if (map_version >= MAP_SOD) {
        int spell_bytes = 9, ability_bytes = 4;
        Some0Bytes<byte> map_allowed_spells (spell_bytes);
        Some0Bytes<byte> map_allowed_abilities (ability_bytes);
        if (! br.Read (map_allowed_spells, spell_bytes)) return 1;
        printf ("AllowedSpells: %d bytes" EOL, spell_bytes);
        if (! br.Read (map_allowed_abilities, ability_bytes)) return 1;
        printf ("AllowedAbilities: %d bytes" EOL, ability_bytes);
    }
    // printf ("prior readRumors Unc Stream Pos: %.8lX" EOL, br.Tell ());
    // readRumors();
    int map_rumors;
    if (! Read (br, map_rumors)) return 1;
    printf ("Rumors       : %d" EOL, map_rumors);
    for (int i = 0; i < map_rumors; i++) {
        printf (" rumor #%3d:", i);
        if (! read_string(br, " Name: ")) return 1;
        printf (" rumor #%3d:", i);
        if (! read_string(br, " Text: ")) return 1;
    }
    // readPredefinedHeroes();
    switch (map_version) {
        case MAP_WOG:
        case MAP_SOD: {
            for (int i = 0; i < MAP_HEROES; i++) {
                byte customized;
                if (! Read (br, customized)) return 1;
                printf ("Customized Hero #%3d: customized: %s" EOL, i,
                    MAP_BOOL(customized));
                if (! customized) continue;
                byte has_exp; // has skills customized, etc.
                if (! Read (br, has_exp)) return 1;
                printf ("Customized Hero #%3d: exp: %s" EOL, i,
                    MAP_BOOL(has_exp));
                if (has_exp) {
                    int custom_exp;
                    if (! Read (br, custom_exp)) return 1;
                    printf ("Customized Hero #%3d: exp q: %d" EOL, i, custom_exp);
                }
                byte has_2nd_skills;
                if (! Read (br, has_2nd_skills)) return 1;
                printf ("Customized Hero #%3d: 2nd skills: %s" EOL, i,
                    MAP_BOOL(has_2nd_skills));
                if (has_2nd_skills) {
                    int s_num;
                    if (! Read (br, s_num)) return 1;
                    printf ("Customized Hero #%3d: num skills: %d" EOL, i, s_num);
                    for (int j = 0; j < s_num; j++) {
                        byte first, second; // ??
                        if (! Read (br, first)) return 1;
                        if (! Read (br, second)) return 1;
                    }
                }

                if (! loadArtifactsOfHero (br, map_version, i)) return 1;

                byte has_bio;
                if (! Read (br, has_bio)) return 1;
                printf ("Customized Hero #%3d: has bio: %s" EOL, i,
                    MAP_BOOL(has_bio));
                if (has_bio) {
                    printf ("Customized Hero #%3d:", i);
                    if (! read_string(br, " Bio: ")) return 1;
                }

                byte sex;
                if (! Read (br, sex)) return 1;
                printf ("Customized Hero #%3d: has bio: %s" EOL, i,
                    MAP_HERO_SEX(sex));

                byte has_spells;
                if (! Read (br, has_spells)) return 1;
                printf ("Customized Hero #%3d: has spells: %s" EOL, i,
                    MAP_BOOL(has_spells));
                if (has_spells) {
                    // readSpells()
                    Some0Bytes<byte> custom_spells (9);
                    if (br.Read (custom_spells, 9)) return 1;
                    printf ("Read custom spells - 9 bytes" EOL);
                }

                byte has_prim_skills;
                if (! Read (br, has_prim_skills)) return 1;
                printf ("Customized Hero #%3d: has prim skills: %s" EOL, i,
                    MAP_BOOL(has_prim_skills));
                if (has_prim_skills)
                    for (int p = 0; p < MAP_PRIMARY_SKILLS; p++) {
                        byte p_skill;
                        if (! Read (br, p_skill)) return 1;
                        printf ("Customized Hero #%3d: prim skill%d: %d" EOL, i,
                            p, p_skill);
                    }
            } // for (int i = 0; i < MAP_HEROES; i++)
        } // case MAP_WOD: case MAP_SOD:
    } // switch (map_version) TODO the switch is pointless; these aren't in RoE

    // readTerrain();
    for (int l = 0; l < (two_level ? 2 : 1); l++)
        for (int y = 0; y < w; y++)
            for (int x = 0; x < w; x++) {
                byte t_type, t_sframe, t_river_type, t_river_dir, t_road_type,
                    t_road_dir, t_tile_flags;
                if (! Read (br, t_type)) return 1;
                // this is the start frame from the .def. the terrain .def
                // contains 1 sprite list, and are named .*tl.def. t_sframe
                // is index in said list; note: the list contains
                // sub-lists (sub-animations for corners and blends);
                // question: why wouldn't I just create a shader?
                if (! Read (br, t_sframe)) return 1;
                if (! Read (br, t_river_type)) return 1;
                if (! Read (br, t_river_dir)) return 1;
                if (! Read (br, t_road_type)) return 1;
                if (! Read (br, t_road_dir)) return 1;
                // bits 0-1 - terrain sprite rotation (0,90,180,270)
                // bits 2-3 - river ^
                // bits 4-5 - road  ^
                // bit    6 - coastal tile
                // bit    7 - haven't met it yet
                // Very bad design: sprite orientation at the res, is
                // implicitly tied to these bits set by the map editor - e.g.
                // if you draw the 0 orientation badly oriented you'll have
                // bug artwork rendered on screen.
                if (! Read (br, t_tile_flags)) return 1;
                printf ("Tile l%d %3d, %3d: type: %s, sframe: #%3d,"
                        " river: %s, rdir: %d, road: %s, rdir: %d,"
                        " flags: %3d" EOL,
                        l, x, y, MAP_TT(t_type), t_sframe,
                        MAP_RIVER(t_river_type), t_river_dir,
                        MAP_ROAD(t_road_type), t_road_dir, t_tile_flags);
            }
    // readDefInfo(); these are the distinct objects on the map
    int obj_num;
    Read (br, obj_num);
    printf ("obj_num      : %d" EOL, obj_num);
    Some0Bytes<int> objects (obj_num);
    for (int i = 0; i < obj_num; i++) {
        MapObject obj (br); objects[i] = obj.EdId;
        Log << "obj[" << Log.Fmt ("%3d", i) << "] " << obj;
        /*if (! read_string (br, "Sprite : ")) return 1;
        Some0Bytes<byte> pass_mask (6);
        Some0Bytes<byte> trig_mask (6);
        Read (br, pass_mask.operator byte * (), 6);
        Log << "  passability:" << print_byte_sequence (pass_mask, 6) << EOL;
        Read (br, trig_mask.operator byte * (), 6);
        Log << "  triggers:" << print_byte_sequence (trig_mask, 6) << EOL;
        ReadSkip (br, 2); // unknown short int
        short int terrain_mask;
        Read (br, terrain_mask);
        Log << "  terrain_mask: " << terrain_mask << EOL;
        int def_id, def_subid;
        if (! Read (br, def_id)) return 1;
        objects[i] = def_id;
        printf ("  def_id: %d" EOL, def_id);
        if (! Read (br, def_subid)) return 1;
        printf ("  def_subid: %d" EOL, def_subid);
        byte def_type, def_print_priority;
        if (! Read (br, def_type)) return 1;
        printf ("  def_type: %d" EOL, def_type);
        if (! Read (br, def_print_priority)) return 1;
        printf ("  def_print_priority: %d" EOL, def_print_priority);
        if (! ReadSkip(br, 16)) return 1;*/ // ?
        // readMsk() - reads the .msk file associated with this .def
        // they are using just the 1st 2 bytes to set mask size (w,h)
    }
    // readObjects(); - object placement
    int obj_ref_num;
    if (! Read (br, obj_ref_num)) return 1;
    printf ("obj_ref_num      : %d" EOL, obj_ref_num);
    for (int i = 0; i < obj_ref_num; i++) {
        printf ("  pos: "); read_pos (br); printf (EOL);
        int def_ref;
        if (! Read (br, def_ref)) return 1;
        printf ("  def_ref: %d" EOL, def_ref);
        if (def_ref < 0 || def_ref >= obj_num)
            return printf ("def_ref out of range"), 1;
        byte unk5[5];
        if (! br.Read (unk5, 5)) return 1;
        printf ("  Unknown byte[5]: {%002X, %002X, %002X, %002X, %002X}" EOL,
            unk5[0], unk5[1], unk5[2], unk5[3], unk5[4]);
        printf ("  obj type (def_id): %3d" EOL, objects[def_ref]);
        // ok, I need the def_array here
        switch (objects[def_ref]) { //TODO to array of function pointers
            case MAP_O_EVENT: {
                readMessageAndGuards (br, map_version);
                int e_exp, e_mana;
                byte e_morale, e_luck;
                if (! Read (br, e_exp)) return 1;
                if (! Read (br, e_mana)) return 1;
                if (! Read (br, e_morale)) return 1;
                if (! Read (br, e_luck)) return 1;
                printf ("  Event: exp: %d, mana: %d, morale: %d, luck: %d" EOL,
                    e_exp, e_mana, e_morale, e_luck);
                readResourses (br);
                for (int p = 0; p < 4; p++) {
                    byte primary_skill;
                    if (! Read (br, primary_skill)) return 1;
                    printf ("  Event: primary skill%d: %d" EOL, p, primary_skill);
                }
                byte ability_num;
                if (! Read (br, ability_num)) return 1;
                printf ("  Event: abilities: %d" EOL, ability_num);
                for (int a = 0; a < ability_num; a++) {
                    byte skill_id, skill_level;
                    if (! Read (br, skill_id)) return 1;
                    if (! Read (br, skill_level)) return 1;
                    printf ("  Event: secondary skill%d: id: %d, levels:%d" EOL,
                        a, skill_id, skill_level);
                }
                byte art_num;
                if (! Read (br, art_num)) return 1;
                printf ("  Event: artifacts: %d" EOL, art_num);
                for (int a = 0; a < art_num; a++) {
                    short int art_id;
                    read_id_ROE8_16 (br, map_version, art_id);
                    printf ("  Event: art%d: id: %d" EOL, a, art_id);
                }
                byte spell_num;
                if (! Read (br, spell_num)) return 1;
                printf ("  Event: spells: %d" EOL, spell_num);
                for (int s = 0; s < spell_num; s++) {
                    byte spell_id;
                    if (! Read (br, spell_id)) return 1;
                    printf ("  Event: spell%d: id: %d" EOL, s, spell_id);
                }
                byte cre_num;
                if (! Read (br, cre_num)) return 1;
                printf ("  Event: creatures: %d" EOL, cre_num);
                readCreatureSet (br, map_version, cre_num);
                if (! ReadSkip (br, 8)) return 1; // ???
                byte availableFor, computerActivate, removeAfter1st;
                if (! Read (br, availableFor)) return 1;
                if (! Read (br, computerActivate)) return 1;
                if (! Read (br, removeAfter1st)) return 1;
                printf ("  Event: avail: %d, comp: %d, rem1st:%d" EOL,
                    availableFor, computerActivate, removeAfter1st);
                if (! ReadSkip (br, 4)) return 1; // ??
            } break; // case MAP_O_EVENT
            case MAP_O_HERO:
            case MAP_O_RANDOM_HERO:
            case MAP_O_PRISON: {
                // readHero
                if (map_version > MAP_ROE) {
                    int hero_id;
                    if (! Read (br, hero_id)) return 1;
                    printf ("  Hero: id: %d" EOL, hero_id);
                }
                byte player_color, subid;
                if (! Read (br, player_color)) return 1;
                printf ("  Hero: color: %d" EOL, player_color);
                if (! Read (br, subid)) return 1;
                printf ("  Hero: subid: %d" EOL, subid);
                byte has_name;
                if (! Read (br, has_name)) return 1;
                printf ("  Hero: has_name: %s" EOL, MAP_BOOL(has_name));
                if (has_name) {
                    printf ("  Hero: ");
                    if (! read_string(br, " Name: ")) return 1;
                }
                if (map_version > MAP_AB) {
                    byte has_exp;
                    if (! Read (br, has_exp)) return 1;
                    printf ("  Hero: has_exp: %s" EOL, MAP_BOOL(has_exp));
                    if (has_exp) {
                        _exp: int hero_exp;
                        if (! Read (br, hero_exp)) return 1;
                        printf ("  Hero: exp: %d" EOL, hero_exp);
                    }
                } else goto _exp;
                byte has_portrait;
                if (! Read (br, has_portrait)) return 1;
                printf ("  Hero: has_portrait: %s" EOL, MAP_BOOL(has_portrait));
                if (has_portrait) {
                    byte portrait_id;
                    if (! Read (br, portrait_id)) return 1;
                    printf ("  Hero: portrait: %d" EOL, portrait_id);
                }
                byte has_sskills;
                if (! Read (br, has_sskills)) return 1;
                printf ("  Hero: has_2ndary_skills: %s" EOL,
                    MAP_BOOL(has_sskills));
                if (has_sskills) {
                    int skill_num;
                    if (! Read (br, skill_num)) return 1;
                    printf ("  Hero: skill_num: %d" EOL, skill_num);
                    for (int s = 0; s < skill_num; s++) {
                        byte first, second;
                        if (! Read (br, first)) return 1;
                        if (! Read (br, second)) return 1;
                        printf ("  Hero: skill%d: 1:%d, 2:%d" EOL, s, first,
                            second);
                    }
                }
                byte has_army;
                if (! Read (br, has_army)) return 1;
                printf ("  Hero: has_army: %s" EOL, MAP_BOOL(has_army));
                if (has_army)
                    readCreatureSet (br, map_version, 7);
                byte formation;
                if (! Read (br, formation)) return 1;
                printf ("  Hero: formation: %d" EOL, formation);

                if (! loadArtifactsOfHero (br, map_version, 0)) return 1;

                byte patrol_radius; // see MAP_NO_PATROL_RADIUS
                if (! Read (br, patrol_radius)) return 1;
                printf ("  Hero: patrol_radius: %d" EOL, patrol_radius);

                if (map_version > MAP_ROE) {
                    byte has_bio;
                    if (! Read (br, has_bio)) return 1;
                    printf ("  Hero: has_bio: %s" EOL, MAP_BOOL(has_bio));
                    if (has_bio)
                        if (! read_string (br, "  Hero: Bio: ")) return 1;
                    byte hero_sex;
                    if (! Read (br, hero_sex)) return 1;
                    printf ("  Hero: sex: %s" EOL, MAP_HERO_SEX(hero_sex));
                }
                if (map_version > MAP_AB) {
                    byte has_spells;
                    if (! Read (br, has_spells)) return 1;
                    printf ("  Hero: has_spells: %s" EOL, MAP_BOOL(has_spells));
                    if (has_spells) { // readSpells 2nd time
                        Some0Bytes<byte> custom_spells (9);
                        if (! br.Read (custom_spells, 9)) return 1;
                        printf ("Read custom spells - 9 bytes" EOL);
                    }
                } else if (MAP_AB == map_version) {
                    byte spell_id;
                    if (! Read (br, spell_id)) return 1;
                    printf ("  Hero: spell: %d" EOL, spell_id);
                }

                if (map_version > MAP_AB) {
                    byte has_pskills;
                    if (! Read (br, has_pskills)) return 1;
                    printf ("  Hero: has_pskills: %s" EOL, MAP_BOOL(has_pskills));
                    if (has_pskills)
                        for (int s = 0; s < MAP_PRIMARY_SKILLS; s++) {
                            byte p_skill;
                            if (! Read (br, p_skill)) return 1;
                            printf ("  Hero: pskill%d: %d" EOL, s, p_skill);
                        }
                }
                if (! ReadSkip (br, 16)) return 1;
            } // case MAP_O_HERO, etc.
            case MAP_O_ARENA: case MAP_O_MERCENARY_CAMP:
            case MAP_O_MARLETTO_TOWER: case MAP_O_STAR_AXIS:
            case MAP_O_GARDEN_OF_REVELATION: case MAP_O_LEARNING_STONE:
            case MAP_O_TREE_OF_KNOWLEDGE:
            case MAP_O_LIBRARY_OF_ENLIGHTENMENT:
            case MAP_O_SCHOOL_OF_MAGIC:
            case MAP_O_SCHOOL_OF_WAR: {} break; // visit once per hero
            case MAP_O_MYSTICAL_GARDEN: case MAP_O_WINDMILL:
            case MAP_O_WATER_WHEEL: {} break; // visit once per week
            case MAP_O_MONOLITH1: case MAP_O_MONOLITH2:
            case MAP_O_MONOLITH3: case MAP_O_SUBTERRANEAN_GATE:
            case MAP_O_WHIRLPOOL: {} break; // teleports
            case MAP_O_CAMPFIRE:
            case MAP_O_FLOTSAM:
            case MAP_O_SEA_CHEST:
            case MAP_O_SHIPWRECK_SURVIVOR:
            {} break; // visit once and remove it from the map
            case MAP_O_TREASURE_CHEST:
            {} break; // same as above 4, but template.subid > 0 is WoG
            case MAP_O_MONSTER:  //Monster
            case MAP_O_RANDOM_MONSTER:
            case MAP_O_RANDOM_MONSTER_L1:
            case MAP_O_RANDOM_MONSTER_L2:
            case MAP_O_RANDOM_MONSTER_L3:
            case MAP_O_RANDOM_MONSTER_L4:
            case MAP_O_RANDOM_MONSTER_L5:
            case MAP_O_RANDOM_MONSTER_L6:
            case MAP_O_RANDOM_MONSTER_L7: {
                if (map_version > MAP_ROE) {
                    int mon_id;
                    if (! Read (br, mon_id)) return 1;
                    printf ("  Mon: id: %d" EOL, mon_id);
                }
                short int mon_qty;
                if (! Read (br, mon_qty)) return 1;
                printf ("  Mon: num: %d" EOL, mon_qty);
                byte mon_disposition;
                if (! Read (br, mon_disposition)) return 1;
                printf ("  Mon: disp: %d" EOL, mon_disposition);
                byte mon_has_msg;
                if (! Read (br, mon_has_msg)) return 1;
                printf ("  Mon: has msg: %s" EOL, MAP_BOOL(mon_has_msg));
                if (mon_has_msg) {
                    if (! read_string (br, "  Mon: Msg: ")) return 1;
                    readResourses (br);
                    short int art_id;
                    read_id_ROE8_16 (br, map_version, art_id);
                    printf ("  Mon: art_id: %d" EOL, art_id);
                }
                byte never_flees, not_growing;
                if (! Read (br, never_flees)) return 1;
                printf ("  Mon: never flees: %s" EOL, MAP_BOOL(never_flees));
                if (! Read (br, not_growing)) return 1;
                printf ("  Mon: not growing: %s" EOL, MAP_BOOL(not_growing));
                if (! ReadSkip (br, 2)) return 1;
            } break; // case MAP_O_MONSTER*
            case MAP_O_OCEAN_BOTTLE:
            case MAP_O_SIGN: {
                if (! read_string(br, " Sign/Bottle: ")) return 1;
                if (! ReadSkip (br, 4)) return 1;
            } break;
            case MAP_O_SEER_HUT: { // readSeerHut
                byte mission_type = 0;
                if (map_version > MAP_ROE)
                    readQuest (br, mission_type);
                else { // MAP_ROE:
                    byte art_id;
                    if (! Read (br, art_id)) return 1;
                    printf ("  Seer: art id: %d" EOL, art_id);
                    // missionType = art_id != 255 ? ART : NONE
                }
                if (mission_type) { // has mission
                    byte reward_type;
                    if (! Read (br, reward_type)) return 1;
                    printf ("  Seer: art id: %s" EOL,
                        MAP_SEER_REWARD(reward_type));
                    switch (reward_type) {
                        case MAP_SEER_EXPERIENCE:
                        case MAP_SEER_MANA_POINTS: {
                            int r_qty;
                            if (! Read (br, r_qty)) return 1;
                            printf ("  Seer: exp/mana: %d" EOL, r_qty);
                        } break;
                        case MAP_SEER_MORALE_BONUS:
                        case MAP_SEER_LUCK_BONUS: {
                            byte r_qty;
                            if (! Read (br, r_qty)) return 1;
                            printf ("  Seer: morale/luck: %d" EOL, r_qty);
                        } break;
                        case MAP_SEER_RESOURCES: {
                            byte res_id;
                            if (! Read (br, res_id)) return 1;
                            printf ("  Seer: res id: %d" EOL, res_id);
                            int res_qty;
                            if (! Read (br, res_qty)) return 1;
                            res_qty &= 0x00ffffff; // TODO check this statement
                            printf ("  Seer: res qty: %d" EOL, res_qty);
                        } break;
                        case MAP_SEER_PRIMARY_SKILL:
                        case MAP_SEER_SECONDARY_SKILL: {
                            byte s_id, s_qty;
                            if (! Read (br, s_id)) return 1;
                            printf ("  Seer: skill id: %d" EOL, s_id);
                            if (! Read (br, s_qty)) return 1;
                            printf ("  Seer: skill qty: %d" EOL, s_qty);
                        } break;
                        case MAP_SEER_ARTIFACT: {
                            short int seer_art_id;
                            read_id_ROE8_16 (br, map_version, seer_art_id);
                            printf ("  Seer: art_id: %d" EOL, seer_art_id);
                        } break;
                        case MAP_SEER_SPELL: {
                            byte spell_id;
                            if (! Read (br, spell_id)) return 1;
                            printf ("  Seer: spell_id: %d" EOL, spell_id);
                        } break;
                        case MAP_SEER_CREATURE: {
                            short int seer_mon_id;
                            read_id_ROE8_16 (br, map_version, seer_mon_id);
                            printf ("  Seer: mon_id: %d" EOL, seer_mon_id);
                            short int seer_mon_qty;
                            if (! Read (br, seer_mon_qty)) return 1;
                            printf ("  Seer: mon_qty: %d" EOL, seer_mon_qty);
                        } break;
                    }; // switch (reward_type)
                    if (! ReadSkip (br, 2)) return 1;
                } else // if (art_id != 255)
                    if (! ReadSkip (br, 3)) return 1;
            } break; // MAP_O_SEER_HUT
            case MAP_O_WITCH_HUT: {
                if (MAP_ROE == map_version) break;
                for (int s = 0; s < 4; s++) {
                    byte potential_skills_mask; // potential skills bitmask
                                                // MAP_SKILL_QUANTITY
                    if (! Read (br, potential_skills_mask)) return 1;
                    printf ("  WHut: potential skills mask: %d" EOL,
                        potential_skills_mask);
                }
            } break;
            case MAP_O_SCHOLAR: {
                byte bonus_type, bonus_id;
                if (! Read (br, bonus_type)) return 1;
                if (! Read (br, bonus_id)) return 1;
                printf ("  Scholar: bonus_type: %d, bonus_id: %d" EOL,
                        bonus_type, bonus_id);
                if (! ReadSkip (br, 6)) return 1; // ??
            } break;
            case MAP_O_GARRISON:
            case MAP_O_GARRISON2: {
                byte player_color;
                if (! Read (br, player_color)) return 1;
                printf ("  Garr: player_color: %d" EOL, player_color);
                if (! ReadSkip (br, 3)) return 1;
                readCreatureSet (br, map_version, 7);
                if (map_version > MAP_ROE) {
                    byte gar_removable;
                    if (! Read (br, gar_removable)) return 1;
                    printf ("  Garr: removable: %s" EOL,
                        MAP_BOOL(gar_removable));
                } // else removable = true; // always removable at RoE
                if (! ReadSkip (br, 8)) return 1;
            } break;
            case MAP_O_ARTIFACT:
            case MAP_O_RANDOM_ART:
            case MAP_O_RANDOM_TREASURE_ART:
            case MAP_O_RANDOM_MINOR_ART:
            case MAP_O_RANDOM_MAJOR_ART:
            case MAP_O_RANDOM_RELIC_ART:
            case MAP_O_SPELL_SCROLL: {
                readMessageAndGuards (br, map_version);
                if (MAP_O_SPELL_SCROLL == objects[def_ref]) {
                    int scroll_id;
                    if (! Read (br, scroll_id)) return 1;
                    printf ("  spell/art: scroll_id: %d" EOL, scroll_id);
                }
            } break;
            case MAP_O_RANDOM_RESOURCE:
            case MAP_O_RESOURCE: {
                readMessageAndGuards (br, map_version);
                int res_qty;
                if (! Read (br, res_qty)) return 1;
                printf ("  res: qty: %d" EOL, res_qty);
                // TODO multiply GOLD * 100 : if(objTempl.subid == Res::GOLD)
                if (! ReadSkip (br, 4)) return 1;
            } break;
            case MAP_O_RANDOM_TOWN:
            case MAP_O_TOWN:
            {
                if (map_version > MAP_ROE) {
                    int t_id;
                    if (! Read (br, t_id)) return 1;
                    printf ("  Town: id: %d" EOL, t_id);
                }
                byte player_color;
                if (! Read (br, player_color)) return 1;
                printf ("  Town: player_color: %d" EOL, player_color);
                byte has_name;
                if (! Read (br, has_name)) return 1;
                printf ("  Town: has name: %s" EOL, MAP_BOOL(has_name));
                if (has_name)
                    if (! read_string (br, "  Town: name: ")) return 1;
                byte has_garr;
                if (! Read (br, has_garr)) return 1;
                printf ("  Town: has garr: %s" EOL, MAP_BOOL(has_garr));
                if (has_garr)
                    if (! readCreatureSet (br, map_version, 7)) return 1;
                byte formation; // of what ?
                if (! Read (br, formation)) return 1;
                printf ("  Town: formation: %d" EOL, formation);
                byte has_custom_buildings;
                if (! Read (br, has_custom_buildings)) return 1;
                printf ("  Town: has custom buildings: %s" EOL,
                    MAP_BOOL(has_custom_buildings));
                if (has_custom_buildings) {
                    Some0Bytes<byte> built_mask (6);
                    Some0Bytes<byte> cant_build_mask (6);
                    if (! br.Read (built_mask, 6)) return 1;
                    if (! br.Read (cant_build_mask, 6)) return 1;
                } else {
                    byte has_fort;
                    if (! Read (br, has_fort)) return 1;
                    printf ("  Town: has fort: %s" EOL, MAP_BOOL(has_fort));
                }
                if (map_version > MAP_ROE)
                    for (int s = 0; s < 9; s++) {
                        byte spell_mask_must;
                        if (! Read (br, spell_mask_must)) return 1;
                        printf ("  Town: spell_mask_must%d: %d" EOL, s,
                            spell_mask_must);
                    }
                for (int s = 0; s < 9; s++) {
                    byte spell_mask_may;
                    if (! Read (br, spell_mask_may)) return 1;
                    printf ("  Town: spell_mask_may%d: %d" EOL, s,
                        spell_mask_may);
                }
                int town_events;
                if (! Read (br, town_events)) return 1;
                printf ("  Town: town_events: %d" EOL, town_events);
                for (int e = 0; e < town_events; e++) {
                    printf ("  Town: event%d ", e);
                    if (! read_string (br, "Name: ")) return 1;
                    printf ("  Town: event%d ", e);
                    if (! read_string (br, "Msg : ")) return 1;
                    if (! readResourses (br)) return 1;
                    byte players; // affected
                    if (! Read (br, players)) return 1;
                    printf ("  Town: event%d players affected: %d" EOL, e,
                        players);
                    if (map_version > MAP_AB) {
                        byte humans_affected;
                        if (! Read (br, humans_affected)) return 1;
                        printf ("  Town: event%d humans affected: %s" EOL, e,
                            MAP_BOOL(humans_affected));
                    }
                    byte computers_affected;
                    if (! Read (br, computers_affected)) return 1;
                    printf ("  Town: event%d computers affected: %s" EOL, e,
                        MAP_BOOL(computers_affected));
                    short int first_occurrence;
                    byte next_occurrence;
                    if (! Read (br, first_occurrence)) return 1;
                    if (! Read (br, next_occurrence)) return 1;
                    printf ("  Town: event%d 1st occurrence: %d" EOL, e,
                        first_occurrence);
                    printf ("  Town: event%d next occurrence: %d" EOL, e,
                        next_occurrence);
                    if (! ReadSkip (br, 17)) return 1; // ??
                    Some0Bytes<byte> new_buildings_mask (6);
                    if (! br.Read (new_buildings_mask, 6)) return 1;
                    for (int c = 0; c < 7; c++) {
                        short int evt_cre;
                        if (! Read (br, evt_cre)) return 1;
                        printf ("  Town: event%d cre%d: %d" EOL, e, c, evt_cre);
                    }
                    if (! ReadSkip (br, 4)) return 1; // ??
                } // for (int e = 0; e < town_events; e++)
                if (map_version > MAP_AB) {
                    byte alignment; // TODO not in the editor ???
                    if (! Read (br, alignment)) return 1;
                    printf ("  Town: alignment: %d" EOL, alignment);
                }
                if (! ReadSkip (br, 3)) return 1; // ??
            } break; // case MAP_O_TOWN
            case MAP_O_MINE:
            case MAP_O_ABANDONED_MINE: {
                byte player_color;
                if (! Read (br, player_color)) return 1;
                printf ("  Mine: player color: %d" EOL, player_color);
                if (! ReadSkip (br, 3)) return 1; // int32 ?
            } break;
            case MAP_O_CREATURE_GENERATOR1:
            case MAP_O_CREATURE_GENERATOR2:
            case MAP_O_CREATURE_GENERATOR3:
            case MAP_O_CREATURE_GENERATOR4: { // identical to MINE above
                byte player_color;
                if (! Read (br, player_color)) return 1;
                printf ("  CreGen: player color: %d" EOL, player_color);
                if (! ReadSkip (br, 3)) return 1; // int32 ?
            } break;
            case MAP_O_REFUGEE_CAMP:
            case MAP_O_WAR_MACHINE_FACTORY: {} break; // CGDwelling
            case MAP_O_SHRINE_OF_MAGIC_INCANTATION:
            case MAP_O_SHRINE_OF_MAGIC_GESTURE:
            case MAP_O_SHRINE_OF_MAGIC_THOUGHT: {
                byte spell_id; // 255 means none, again
                if (! Read (br, spell_id)) return 1;
                printf ("  Shrine/camp: id: %d" EOL, spell_id);
                if (! ReadSkip (br, 3)) return 1;
            } break;
            case MAP_O_PANDORAS_BOX: {
                readMessageAndGuards (br, map_version);
                int pan_exp, pan_mana;
                byte pan_morale, pan_luck;
                if (! Read (br, pan_exp)) return 1;
                if (! Read (br, pan_mana)) return 1;
                if (! Read (br, pan_morale)) return 1;
                if (! Read (br, pan_luck)) return 1;
                printf ("  Pandora: exp: %d, mana: %d, morale: %d, luck: %d" EOL,
                    pan_exp, pan_mana, pan_morale, pan_luck);
                readResourses (br);
                for (int s = 0; s < MAP_PRIMARY_SKILLS; s++) {
                    byte p_skill;
                    if (! Read (br, p_skill)) return 1;
                    printf ("  Pandora: pskill%d: %d" EOL, s, p_skill);
                }
                byte abilities_num;
                if (! Read (br, abilities_num)) return 1;
                printf ("  Pandora: abilities: %d" EOL, abilities_num);
                for (int a = 0; a < abilities_num; a++) {
                    byte skill, level;
                    if (! Read (br, skill)) return 1;
                    if (! Read (br, level)) return 1;
                    printf ("  Pandora: ability%d: skill: %d, level: %d" EOL,
                        a, skill, level);
                }
                byte art_num;
                if (! Read (br, art_num)) return 1;
                printf ("  Pandora: artifacts: %d" EOL, art_num);
                for (int a = 0; a < art_num; a++) {
                    short int art_id; //DONE this fragment repeats one too many
                    read_id_ROE8_16 (br, map_version, art_id);
                    printf ("  Pandora: art%d: id: %d" EOL, a, art_id);
                }
                byte spell_num;
                if (! Read (br, spell_num)) return 1;
                printf ("  Pandora: spells: %d" EOL, spell_num);
                for (int a = 0; a < spell_num; a++) {
                    byte spell_id;
                    if (! Read (br, spell_id)) return 1;
                    printf ("  Pandora: spell%d: id: %d" EOL, a, spell_id);
                }
                byte cre_num;
                if (! Read (br, cre_num)) return 1;
                printf ("  Pandora: cre: %d" EOL, cre_num);
                if (! readCreatureSet (br, map_version, cre_num)) return 1;
                if (! ReadSkip (br, 8)) return 1;
            } break; // case MAP_O_PANDORAS_BOX
            case MAP_O_GRAIL: {
                int g_radius;
                if (! Read (br, g_radius)) return 1;
                printf ("  Grail: radius: %d" EOL, g_radius);
            } break;
            case MAP_O_RANDOM_DWELLING: //same as castle + level range
            case MAP_O_RANDOM_DWELLING_LVL: //same as castle, fixed level
            case MAP_O_RANDOM_DWELLING_FACTION: {
                //TODO not decoded properly; there are things missing - like
                // "alignment"; place random dwelling (all terrain objects)
                // and check
                int player_color; //TODO why 32 bits ?
                if (! Read (br, player_color)) return 1;
                printf ("  Dwelling: player color: %d" EOL, player_color);
                if (objects[def_ref] != MAP_O_RANDOM_DWELLING_FACTION) {
                    int d_id; // This probably contains "alignment"
                    if (! Read (br, d_id)) return 1;
                    printf ("  Dwelling: id: %d" EOL, d_id);
                    if (! d_id) { //TODO there is an error here
                        // "same as" can have as many castles as there are
                        // random castles on the map
                        byte d_castle0, d_castle1; // why 2?
                        if (! Read (br, d_castle0)) return 1;
                        if (! Read (br, d_castle1)) return 1;
                        printf ("  Dwelling: castle0?: %d" EOL, d_castle0);
                        printf ("  Dwelling: castle1?: %d" EOL, d_castle1);
                    }
                }
                if (objects[def_ref] != MAP_O_RANDOM_DWELLING_LVL) {
                    byte min_level, max_level;
                    if (! Read (br, min_level)) return 1;
                    if (! Read (br, max_level)) return 1;
                    printf ("  Dwelling: min lvl: %d, max lvl: %d" EOL,
                        min_level, max_level);
                }
            } break; // MAP_O_RANDOM_DWELLING
            case MAP_O_QUEST_GUARD: {
                byte mission_type = 0;
                readQuest (br, mission_type);
            } break;
            case MAP_O_FAERIE_RING:
            case MAP_O_SWAN_POND:
            case MAP_O_IDOL_OF_FORTUNE:
            case MAP_O_FOUNTAIN_OF_FORTUNE:
            case MAP_O_RALLY_FLAG:
            case MAP_O_OASIS:
            case MAP_O_TEMPLE:
            case MAP_O_WATERING_HOLE:
            case MAP_O_FOUNTAIN_OF_YOUTH:
            case MAP_O_BUOY:
            case MAP_O_MERMAID:
            case MAP_O_STABLES: {} break; // bonus item
            case MAP_O_MAGIC_WELL: {} break; // ??
            case MAP_O_COVER_OF_DARKNESS:
            case MAP_O_REDWOOD_OBSERVATORY:
            case MAP_O_PILLAR_OF_FIRE: {} break; // observatory?
            case MAP_O_CORPSE:
            case MAP_O_LEAN_TO:
            case MAP_O_WAGON:
            case MAP_O_WARRIORS_TOMB: {} break ; // visit once?
            case MAP_O_BOAT: {} break; // ??
            case MAP_O_SIRENS: {} break; // ??
            case MAP_O_SHIPYARD: { // same as lighthouse below
                int player_color;
                if (! Read (br, player_color)) return 1;
                printf ("  Shipyard: player color: %d" EOL, player_color);
            } break;
            case MAP_O_HERO_PLACEHOLDER: {
                byte player_color, hero_tid;
                if (! Read (br, player_color)) return 1;
                printf ("  Hero: player color: %d" EOL, player_color);
                if (! Read (br, hero_tid)) return 1;
                printf ("  Hero: type: %d" EOL, hero_tid);
                if (255 == hero_tid) { //TODO DEFAULT
                    byte hero_power;
                    if (! Read (br, hero_power)) return 1;
                    printf ("  Hero: power: %d" EOL, hero_power);
                }
            } break;
            case MAP_O_KEYMASTER: {} break;
            case MAP_O_BORDERGUARD: {} break;
            case MAP_O_BORDER_GATE: {} break;
            case MAP_O_EYE_OF_MAGI:
            case MAP_O_HUT_OF_MAGI: {} break;
            case MAP_O_CREATURE_BANK:
            case MAP_O_DERELICT_SHIP:
            case MAP_O_DRAGON_UTOPIA:
            case MAP_O_CRYPT:
            case MAP_O_SHIPWRECK: {} break;
            case MAP_O_PYRAMID: {} break;
            case MAP_O_CARTOGRAPHER: {} break;
            case MAP_O_MAGIC_SPRING: {} break;
            case MAP_O_DEN_OF_THIEVES: {} break;
            case MAP_O_OBELISK: {} break;
            case MAP_O_LIGHTHOUSE: {
                int player_color;
                if (! Read (br, player_color)) return 1;
                printf ("  Lighthouse: player color: %d" EOL, player_color);
            } break;
            case MAP_O_ALTAR_OF_SACRIFICE:
            case MAP_O_TRADING_POST:
            case MAP_O_FREELANCERS_GUILD:
            case MAP_O_TRADING_POST_SNOW: {} break;
            case MAP_O_UNIVERSITY: {} break;
            case MAP_O_BLACK_MARKET: {} break;
            case MAP_O_FAVORABLE_WINDS: {} break;
            default: printf ("any other object ??" EOL); break;
        } // big def_id switch
    }

    // readEvents();
    int event_num;
    if (! Read (br, event_num)) return 1;
    printf ("Events: %d" EOL, event_num);
    for (int i = 0; i < event_num; i++) {
        printf ("  Event%d:", i); read_string (br, " Name: ");
        printf ("  Event%d:", i); read_string (br, " Msg : ");
        readResourses (br);
        byte e_players;
        if (! Read (br, e_players)) return 1;
        printf ("  Event%d: players: %d" EOL, i, e_players);
        if (map_version > MAP_AB) {
            byte human_affected;
            if (! Read (br, human_affected)) return 1;
            printf ("  Event%d: human_affected: %d" EOL, i, human_affected);
        }
        byte computer_affected;
        if (! Read (br, computer_affected)) return 1;
        printf ("  Event%d: computer_affected: %d" EOL, i, computer_affected);
        short int first_occ;
        byte next_occ;
        if (! Read (br, first_occ)) return 1;
        if (! Read (br, next_occ)) return 1;
        printf ("  Event%d: first_occ: %d" EOL, i, first_occ);
        printf ("  Event%d: next_occ: %d" EOL, i, next_occ);
        if (! ReadSkip (br, 17)) return 1;
    }
TELL_POS
    return 0;
}

// nice mountain view you, virus
