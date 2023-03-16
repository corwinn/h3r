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

#ifndef _H3R_NEWGAME_H_
#define _H3R_NEWGAME_H_

#include "h3r.h"
#include "h3r_renderengine.h"
#include "h3r_dialogwindow.h"
#include "h3r_string.h"
#include "h3r_event.h"
#include "h3r_scrollbar.h"
#include "h3r_criticalsection.h"
#include "h3r_asyncfsenum.h"
#include "h3r_spritecontrol.h"
#include "h3r_map.h"
#include "h3r_dll.h"
#include "h3r_sort.h"

H3R_NAMESPACE

// defined by SCSelBck.pcx
#define H3R_VISIBLE_LIST_ITEMS 18

// This one doesn't get drop shadow?!
// And it isn't even centered?! How many thing one didn't usually notice :)
//
// New game dialog:
//
//  - tab: "Show Advanced Options" (left): ADVOPTBK.PCX
//    - item: ADOP(B|G|O|P|R|S(pink)|T|Y(tan))PNL.PCX
//    - (starting bonus)left arrow: ADOPLFA.DEF
//    -                             ADOPRTA.DEF - why?! this won't be loaded
//    - (starting bonus image): SCNRSTAR.def; whats this: CampStar.def ?
//    - flag buttons: AOFLGB(B|G|O|P|R|S(pink)|T|Y(tan)).DEF
//    - probably used at the tool tip ("Starting Town"): CPRSMALL.def
//    - castle icons (for the Starting Town column): ITPA.def
//    - Starting Hero: can't select: hpsrand6.pcx; other variations:
//      2 dies with "?": HPSRAND4.PCX ?; heroes with "?": hpsrand.pcx ?
//  [] main: GSelPop1.pcx (370x585)
//    [] background: gamselb(0|1).pcx (w: 800, h: 600) - randomly chosen
//    [] - another one: ScnrBeg.def   (w: 166, h:  40) ?!
//         button "begin": GSPBGIN.DEF  (w: 166, h:  40)
//    [] - button "back" : SCNRBACK.def (w: 166, h:  40) (n & s only)
// <> This mess speaks of a 3rd party that is ordering it <>
//    [] the odd long buttons (the tab switch ones): GSPBUT2.DEF ( has no < );
//       ( has < ) : GSPBUTT.DEF      (w: 200, h:  20)
//    [] diff. buttons: GSPBUT[3;7].DEF - 0-4 (w:  30, h:  46)
//    [] mini-static-flags: itgflags.def      (w:  15, h:  20)
//    [] quad arrow down button: ScnrBDn.def; correct and pointless: ScnrBUp.def
//       the other two pointless ones: ScnrBLf.def, ScnrBRt.def
//                                           (w:  16, h:  16)
//    [] scroll-bar middle button: ScnrBSl.def(w:  16, h:  16)
//    [] another scroll-bar set: SCXLBUT.def(w:  44, h:  33), SLIDBUV.def(w:  16,
//       h:  16), SLIDEBUH.def(w:  16, h:  16), SLIDEBUV.def(w:  16, h:  16) ?!
//    [] top-left mini-icon of the selected map size: ScnrMpSz.def
//                                           (w:  29, h:  23)
//    [] victory condition mini-icon: SCNRVICT.def (w:  29, h:  21)
//    [] loss condition mini-icon: SCNRLOSS.def    (w:  29, h:  21)
//  - tab: "Random Map": RanMapBk.pcx
//  [] tab: "Show Available Scenarios" SCSelBck.pcx
//    [] filter by size buttons: RanSiz(L|M|S|X).def; "All": SCALBUT.DEF
//      [] alt1: SC L GBUT.DEF ; alt2: SC M DBUT.DEF ?!
//      [] alt3: SC S MBUT.DEF; alt4: SC X LBUT.DEF
//    [] sort by Map Version column header button: SCButCp.DEF
//    - map version icon: ScSelC.def
//    [] sort by Player Num column header button: SCBUTT1.DEF
//    [] sort by Map Size column header button: SCBUTT2.DEF
//    [] sort by Map Name column header button: SCBUTT3.DEF
//    [] sort by Victory Con column header button: SCBUTT4.DEF
//    [] sort by Loss Con column header button: SCBUTT5.DEF
//
//   I can't find the tool-tip windows yet.
//
//  - quad arrows on brown background: "ComSlide.def" ?!
//  - load progress bars: loadprog.def
//  - load background: loadbar.pcx
#undef public
class NewGameDialog final : public DialogWindow, public IHandleEvents
#define public public:
{
    // private List<int> _re_keys {};
    private bool _has_dr {};
    private DialogResult _dr {};
    private int _t {}, _l {}; // top, left - of the persistent (right) part

    // List item: details
    private Label         * _lid_sname_lbl   {};
    private SpriteControl * _lid_map_size_sc {};
    private Label         * _lid_sdescr_lbl  {};
    private SpriteControl * _lid_vcon_sc     {};
    private Label         * _lid_vcon_lbl    {};
    private SpriteControl * _lid_lcon_sc     {};
    private Label         * _lid_lcon_lbl    {};
    private Label         * _lid_diff_lbl    {};
    private Label         * _lid_rating_lbl  {};

    //PERHAPS this shall be a separate class: UserControl
    private Control * _tab_avail_scen {};
    private List<int> _tab_avail_scen_keys {};
    private ScrollBar * _tab_avail_scen_vs {};

    public NewGameDialog(Window * base_window);
    public ~NewGameDialog() override;
    public DialogResult ShowDialog();//TODO virtual at dialogwindow

    protected void OnKeyUp(const EventArgs &) override;

    private void ToggleAvailScen(EventArgs *);
    private void ToggleRndScen(EventArgs *);
    private void ToggleAdvOpt(EventArgs *);
    private void Begin(EventArgs *);
    private void Back(EventArgs *);

    // The difficulty group; I don't see a reason to create a class yet.
    private void BtnGroup(EventArgs *);
    private Button * _btn_grp[5] {}; // refs

    // H3R_VISIBLE_LIST_ITEMS
    private struct ListItem final //TODO shouldn't this become a Control?
    {
        Control * Base {};
        Label         * Players; // Total/Human
        Label         * Size;    // XL/L/M/S
        SpriteControl * Version; // sprite
        Label         * Name;    //
        SpriteControl * Victory; // sprite
        SpriteControl * Loss;    // sprite
        String        VConText {}; // these are used by 1 UI label
        String        LConText {}; //
        //
        Map * Map {}; //TODO remove me from here
        ListItem(Control * base, Point p_loc, Point sz_loc, Point nm_loc,
            Point ver_loc, Point vcon_loc, Point lcon_loc)
            : Base{base}
        {
            H3R_CREATE_OBJECT(Players, Label) {"8/8", "smalfont.fnt", p_loc,
                base, H3R_TEXT_COLOR_WHITE, false, Point {34, 26}};
            H3R_CREATE_OBJECT(Size, Label) {"XL", "smalfont.fnt", sz_loc, base,
                H3R_TEXT_COLOR_WHITE, false, Point {36, 26}};

            H3R_CREATE_OBJECT(Version, SpriteControl) {"ScSelC.def", base,
                ver_loc};
            Version->Map ("ScSelCre.pcx", 0x0e);
            Version->Map ("ScSelCab.pcx", 0x15);
            Version->Map ("ScSelCsd.pcx", 0x1c);

            H3R_CREATE_OBJECT(Name, Label) {"-", "smalfont.fnt", nm_loc, base,
                H3R_TEXT_COLOR_WHITE, false, Point {186, 26}};

            H3R_CREATE_OBJECT(Victory, SpriteControl) {"SCNRVICT.def", base,
                vcon_loc};
            // map to Map.VCon                  "vcdesc.txt": line "key":
            Victory->Map ("ScnrVLaa.pcx",  1); // "Acquire Artifact"
            Victory->Map ("ScnrVLac.pcx",  2); // "Accumulate Creatures"
            Victory->Map ("ScnrVLar.pcx",  3); // "Accumulate Resources"
            Victory->Map ("ScnrVLut.pcx",  4); // "Upgrade Town"
            Victory->Map ("ScnrVLbg.pcx",  5); // "Build a Grail Structure"
            Victory->Map ("ScnrVLdh.pcx",  6); // "Defeat Hero"
            Victory->Map ("ScnrVLct.pcx",  7); // "Capture Town"
            Victory->Map ("ScnrVLdm.pcx",  8); // "Defeat Monster"
            Victory->Map ("ScnrVLfg.pcx",  9); // "Flag All Creature Dwellings"
            Victory->Map ("ScnrVLfm.pcx", 10); // "Flag All Mines"
            Victory->Map ("ScnrVLta.pcx", 11); // "Transport Artifact"
            Victory->Map ("ScnrVLwn.pcx",  0); // "Defeat All Enemies"

            H3R_CREATE_OBJECT(Loss, SpriteControl) {"SCNRLOSS.def", base,
                lcon_loc};
            // map to Map.LCon              "lcdesc.txt": line "key":
            Loss->Map ("ScnrVLlt.pcx", 1); // "Lose Town"
            Loss->Map ("ScnrVLlh.pcx", 2); // "Lose Hero"
            Loss->Map ("ScnrVLtl.pcx", 3); // "Time Expires"
            Loss->Map ("ScnrVLls.pcx", 0); // "Lose All Your Towns and Heroes"

            Victory->Show (0);
            Loss->Show (0);
            // Everything is hidden
            Control * all[6] = {Players, Size, Version, Name, Victory, Loss};
            for (auto c : all) c->SetHidden (true);
        }// ListItem::ListItem()
        void SetMap(class Map * map);
    };// ListItem
    private List<ListItem *> _map_items {H3R_VISIBLE_LIST_ITEMS};
    // private ListItem * _selected {};
    private Map * _selected_map {};
    private using Node = LList<Map *>;
    // Encapsulates pretty complicated state of a map list being sorted and
    // updated and rendered in real-time:
    //   * thread 1: an async. file enumerator:
    //     * adding maps
    //     * performing multi-key sort when certain amount of maps is added
    //   * thread 2: main: rendering a fragment of the list while complying with
    //     user interaction:
    //     * select a map
    //     * scroll up/down the map list
    //     * stop thread 1 when the user decides to close the dialog
    // Why? Because if you have many maps (or slow, or under a heavy load HDD)
    // you can start playing prior all of them are being enumerated.
    // Because a program should handle things gracefully. Because there is no
    // objective reason to not do so.
#undef public
    private class MapList final : public ISortable<Node>
#define public public:
    {
        private Node * _head {}, * _tail {};
        private Node * _visible {};
        private int _cnt {};
        public ~MapList()
        {
            // printf ("destroying %d nodes\n", _cnt);
            while (_tail) {
                auto n = _tail->Prev ();
                _tail = _tail->Delete (); _cnt--; // keep the LL up to date
                H3R_DESTROY_OBJECT(_tail->Data, Map)
                H3R_DESTROY_OBJECT(_tail, LList<class Map *>)
                // printf (" destr %d nodes\n", _cnt);
                _tail = n;
            }
            H3R_ENSURE(0 == _cnt, "bug at an LL")
            _head = _tail = _visible = 0;
        }

        // ISortable<Node>
        //TODO testme
        Node * first() override { return _head; }
        Node * next(Node * a) override { return a->Next (); }
        bool cmp(Node * a, Node * b) override
        {
            //TODO This compiles w/o String having "operator>=" what the?
            // return a->Data->Name () >= b->Data->Name ();

            // The default; TODO the other keys; the multi-key always has
            //                   Name() as 2nd priority
            //TODO their sort is case-insensitive: "n" is prior "M"
            auto k1 = String::Format ("%d%s", a->Data->Version (),
                a->Data->Name ().ToLower ().AsZStr ());
            auto k2 = String::Format ("%d%s", b->Data->Version (),
                b->Data->Name ().ToLower (). AsZStr ());
            auto len = k1.Length () < k2.Length () ? k1.Length ()
                : k2.Length ();
            return OS::Memcmp (k1.AsZStr (), k2.AsZStr (), len) >= 0;
        }
        // insert "b" prior "a"; b != null ; nullptr as "b" will get you a null
        // dereference; when "a" is nullptr "b" is the new last one
        void insert(Node * a, Node * b) override
        {
            // printf("<> prior: a: %p, b: %p, v: %p\n", a, b, _visible);

            // it is a little bit complicated: _visible shall stay at nth node
            Node * b_prev = b->Prev ();

            if (b == _tail) _tail = b->Prev ();
            if (b == _head) _head = b->Next ();
            if (nullptr == a) {
                if (b != _tail) _tail = _tail->InsertAfter (b->Delete ());
            }
            else {
                a->Insert (b->Delete ());
                if (a == _head) _head = b;
            }

            // _visible stays at nth node
            if (_visible == a) _visible = b;
            else if (_visible == b) { if (b_prev) _visible = b_prev; }
            else { if (_visible->Prev ()) _visible = _visible->Prev (); }

            // printf("<> after: a: %p, b: %p, v: %p\n", a, b, _visible);
            // Battle-field test unit.
            // Diagram-warrior: nodes_lls_sorting_and_rt-ui.dia
            // Bug caught in 3 minutes: incorrect handling of "v" is neither "a"
            // nor "b" case.
            // When dealing with nodes (LL, Trees, Nets, Graphs, Whatever) use
            // diagrams, and common sense, or face serious time waste.
            // This here is simple; try understanding the linux memory manager.
            /*
            H3R_ENSURE(_visible == _head, "bug hunter: back to the white-board")
            */
        }// ISortable.insert()

        public inline void Add(class Map * m)
        {
            Node * n {};
            H3R_CREATE_OBJECT(n, Node) {}; _cnt++;
            n->Data = m;
            if (! _head) _head = _tail = _visible = n;
            else {
                // As it happens it does matter if inserting at _head or not: in
                // a live update scene this is causing unsorted items being
                // displayed, when sorting is partitioned, resulting in top item
                // staying still while next n-1 are constantly being changed:
                // _head->InsertAfter (n); if (! _tail) _tail = n;

                // This renders better, because the unsorted partition is being
                // constructed at ll.tail while a few at ll.head are being
                // displayed.
                _tail = _tail->InsertAfter (n);
            }
        }
        public inline class Map * FirstMap() const { return this->Map (_head); }
        public inline Node * FirstVisible() const { return _visible; }
        public inline Node * First() const { return _head; }
        public inline class Map * Map(Node * n) const
        {
            return nullptr != n ? n->Data : nullptr;
        }
        public inline int Count() const { return _cnt; }
        public inline void SetVisible(Node * n)
        {
            if (nullptr != n) _visible = n;
        }
    };// MapList
    private MapList _maps {};

    private OS::CriticalSection _map_gate {};
    // Does the original do recursive scan: no.
    // Does it do an async. scan: no.
    // Does this remake do the above: yes.
    //LATER think about an option to show progress somewhere
    private class MapListInit final // header_only = true
    {
        private MapList & MList;
        private OS::CriticalSection & MapListGate;
        private AsyncFsEnum<MapListInit> _subject;
        private int _files {}, _dirs {};
        // private int _supported_maps {};
        // AsyncFsEnum<MapListInit> handler
        private bool HandleItem(
            const H3R_NS::AsyncFsEnum<MapListInit>::EnumItem & itm)
        {
            if (itm.IsDirectory) return _dirs++, ! Stop;
            _files++;
            if (! itm.FileName.ToLower ().EndsWith (".h3m")) return ! Stop;
            Dbg.Enabled = false;
                bool header_only {};
                Map * map {};
                H3R_CREATE_OBJECT(map, Map) {itm.Name, header_only = true};
            Dbg.Enabled = true;
            //TODO check: the game skips nameless maps it seems
            if (! map->SupportedVersion () || map->Name ().Empty ()) { // SOD
                Dbg << "Skipped unsupported map (" << map->VersionName ()
                    << ") : " << itm.Name << EOL;
                H3R_DESTROY_OBJECT(map, Map)
                return ! Stop;
            }
            {
                __pointless_verbosity::CriticalSection_Acquire_finally_release
                    ___ {MapListGate};
                MList.Add (map);
                /*printf ("add: map[%3d]: %s\n", _supported_maps++,
                        map->Name ().AsZStr ());*/
                //TODO name this SORT_PARTITION_SIZE or something
                if (MList.Count () % 200 == 0)
                    sort (MList);
            }
            return ! Stop;
        }
        // AsyncFsEnum<MapListInit> handler
        private void Done()
        {
            //TODO Log::Unbuffered
            printf ("Found: %d maps" EOL, MList.Count ());
            // the file enum thread still - so don't invite race conditions
            __pointless_verbosity::CriticalSection_Acquire_finally_release
                ___ {MapListGate};
            sort (MList);
            /*auto n = MList.First ();
            int id = 0;
            while (nullptr != n) {
                printf ("map[%3d]: %s\n", id++, n->Data->Name ().AsZStr ());
                n = n->Next ();
            }*/
        }
        public MapListInit(String p, MapList & l, OS::CriticalSection & lg);
        public bool Complete() const { return _subject.Complete (); }
        public int Files() const { return _files; }
        public int Directories() const { return _dirs; }
        public bool Stop {false};
    } _scan_for_maps; // MapListInit
    private void SetListItem(ListItem *);
    private void SetListItem(Map *);

    private NewGameDialog::ListItem * ChangeSelected(Map *);

    private bool _changed {}; // temporary - until TabControl
    private bool _user_changed_selected_item {};
    private int _prev_maps_count {};
    private void OnRender() override;

    // The game changes selection based on mouse down location, but on mouse up
    // event; looks inconsistent; this remake will act on mouse down.
    private void OnMouseDown(const EventArgs &) override;
    private void OnKeyDown(const EventArgs &) override;

    private void Scroll(EventArgs *);

    // model: _maps; view: _map_items, _lid_.*
    private void Model2View();

    // The original does something odd: if you select a map and drag the scroll-
    // bar in such a way that the selected one goes off-screen, selecting
    // another map using the arrow up/down keys won't focus (make it visible)
    // the selected one.
    // This behavior won't be replicated. Changing selection will focus it (it
    // will show it as the top one.
    //
    // Page Up/Down doesn't change the selected map - it just does what the
    // vertical scrollbar does.
    //
    // Up/Down arrow changes the selected map and scrolls up/down when the
    // selected item becomes a boundary item (top one + arrow up; bottom one +
    // arrow down).
    private inline int SelectedMap2ListItemIndex() // long name on purpose
    {
        for (int i = 0; i < _map_items.Count (); i++)
            if (_map_items[i]->Map == _selected_map) return i;
        return -1;
    }
};// NewGameDialog

NAMESPACE_H3R

#endif