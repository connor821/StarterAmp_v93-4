#pragma once
// b46: cable routing. The signal path is a graph: pedals, the amps and the cab are nodes, and cables join an output jack to an input jack.
//
//   Nodes   0..27   the pedals (the same numbers as the old effect blocks; 10 and 13, the Doubler and the Widener, stay fixed at the very end and are not nodes)
//           28      NAM pedal 1 (the neural pedal in front of the drives and the amp)
//           29      Amp A  (in: 0 input, 1 loop return;  out: 0 output, 1 loop send)
//           30      Amp B  (in: 0 input;  out: 0)
//           31      Cab    (in: 0 from Amp A, 1 from Amp B;  out: 0)       the four cab blocks stay inside it
//           32..34  Crossover 1 to 3 (in: 0;  out: 0 lows, 1 highs)
//           35      Left / right split (in: 0;  out: 0 left on both sides, 1 right on both sides)
//           36      Input,  37 Output
//
// Several cables into one jack are added together. A node with no way in from the Input, or no way out to the Output, is not run.
// A cable has a level, a polarity, a pan (only where the signal is stereo, after the cab) and a channel mask (both / left only / right only).
// The loop is just a path that leaves Amp A's send jack and comes back to its return jack: whatever lies on that path runs inside the amp,
// between the preamp and the power stage. Everything here is plain data (no allocation), so the audio thread can compile a patch itself.
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace PatchGraph
{
    constexpr int kPedals = 28;                         // b55: 27 is the Multiband Saturator; the nodes after the pedals moved up by one
    enum Node : int { kNamP1 = 28, kAmpA = 29, kAmpB = 30, kCab = 31, kXo1 = 32, kXo2 = 33, kXo3 = 34, kLr = 35, kIn = 36, kOut = 37, kNodes = 38 };
    constexpr int kMaxCables = 96, kMaxIn = 10;
    constexpr float kMuteDb = -60.0f;                  // a cable level at or below this is silence

    inline bool isPedal (int n)  { return n >= 0 && n < kPedals && n != 10 && n != 13; }
    inline bool isNode (int n)   { return isPedal (n) || (n >= kNamP1 && n < kNodes); }
    inline bool isXover (int n)  { return n >= kXo1 && n <= kXo3; }
    inline int  outPorts (int n) { return n == kAmpA || isXover (n) || n == kLr ? 2 : n == kOut ? 0 : 1; }
    inline int  inPorts (int n)  { return n == kAmpA || n == kCab ? 2 : n == kIn ? 0 : 1; }

    struct Cable
    {
        int8_t from = 0, fp = 0, to = 0, tp = 0;
        uint8_t pol = 0;                               // 1 = flip polarity
        uint8_t chan = 0;                              // 0 both channels, 1 left only, 2 right only (what this cable adds at its destination)
        float levelDb = 0.0f, pan = 0.0f;
    };
    struct Data
    {
        int n = 0;
        Cable c[kMaxCables];
        float xoHz[3] = { 800.0f, 800.0f, 800.0f };    // the three crossovers' split frequencies
    };

    // ---------------------------------------------------------------------------------------------------------------------
    // compiled form, read by the audio thread
    // ---------------------------------------------------------------------------------------------------------------------
    struct In { uint8_t src = 0, sp = 0, chan = 0; bool usePan = false; float lvl = 1.0f, panL = 1.0f, panR = 1.0f; };
    struct Step { uint8_t node = 0; uint8_t n[2] = { 0, 0 }; In in[2][kMaxIn]; };
    struct Plan
    {
        int nSteps = 0, nLoop = 0;
        Step steps[kNodes];                            // the main run, in order
        Step loop[kNodes];                             // what runs inside Amp A (between its preamp and its power stage)
        uint8_t group[kNodes] = {};                    // 0 mono, before the cab;  1 stereo, after the cab;  2 inside the amp's loop
        bool active[kNodes] = {}, inLoop[kNodes] = {};
        bool cableOk[kMaxCables] = {};                 // false: the cable is not used (bad ends, would loop back, joins the loop from outside, ...)
        bool hasLoop = false, loopHasBlocks = false, tremInLoop = false;
        int nTail = 0; uint8_t tail[2] = {};           // drives that sit right in front of the amp(s), in signal order: they are built into the amp's oversampled path
        float xoHz[3] = { 800.0f, 800.0f, 800.0f };
    };

    inline float dbToGain (float db) { return db > -100.0f ? std::pow (10.0f, db * 0.05f) : 0.0f; }     // the same formula as juce::Decibels

    inline void panGains (float pan, float& pl, float& pr)
    {
        const float p = pan < -1.0f ? -1.0f : pan > 1.0f ? 1.0f : pan;
        const float th = (p + 1.0f) * 0.78539816f;                         // constant power, unity in the centre
        pl = 1.41421356f * std::cos (th);  pr = 1.41421356f * std::sin (th);
    }

    inline bool cableShapeOk (const Cable& c)
    {
        if (! isNode (c.from) || ! isNode (c.to)) return false;
        if (c.from == c.to && ! (c.from == kAmpA && c.fp == 1 && c.tp == 1)) return false;        // only Amp A's send can go straight to its own return
        if (c.from == kOut || c.to == kIn) return false;
        if (c.fp < 0 || c.fp >= outPorts (c.from) || c.tp < 0 || c.tp >= inPorts (c.to)) return false;
        return true;
    }

    inline void compile (const Data& d, Plan& p)
    {
        p = Plan();
        p.nSteps = p.nLoop = 0;
        for (int i = 0; i < 3; ++i) p.xoHz[i] = d.xoHz[i];
        const int nc = d.n < kMaxCables ? d.n : kMaxCables;
        bool ok[kMaxCables] = {};
        for (int i = 0; i < nc; ++i)
        {
            const Cable& c = d.c[i];
            ok[i] = cableShapeOk (c);
            for (int j = 0; ok[i] && j < i; ++j)
                if (ok[j] && d.c[j].from == c.from && d.c[j].fp == c.fp && d.c[j].to == c.to && d.c[j].tp == c.tp) ok[i] = false;    // the same cable twice
        }

        // ---- the loop: nodes that can be reached from Amp A's send jack and that lead back to its return jack
        bool fwd[kNodes] = {}, bwd[kNodes] = {};
        for (bool again = true; again;)
        {
            again = false;
            for (int i = 0; i < nc; ++i)
            {
                if (! ok[i]) continue;
                const Cable& c = d.c[i];
                const bool seed = c.from == kAmpA && c.fp == 1;
                if ((seed || (fwd[c.from] && c.from != kAmpA)) && c.to != kAmpA && ! fwd[c.to]) { fwd[c.to] = true; again = true; }
            }
        }
        for (bool again = true; again;)
        {
            again = false;
            for (int i = 0; i < nc; ++i)
            {
                if (! ok[i]) continue;
                const Cable& c = d.c[i];
                const bool seed = c.to == kAmpA && c.tp == 1;
                if ((seed || (bwd[c.to] && c.to != kAmpA)) && c.from != kAmpA && ! bwd[c.from]) { bwd[c.from] = true; again = true; }
            }
        }
        for (int n = 0; n < kNodes; ++n)
            p.inLoop[n] = fwd[n] && bwd[n] && n != kAmpA && n != kAmpB && n != kCab && n != kIn && n != kOut;
        for (int n = 0; n < kNodes; ++n) if (p.inLoop[n]) p.hasLoop = true;

        // which cables belong to the loop, and drop the ones that cross its edge
        bool isLoopCable[kMaxCables] = {};
        for (int i = 0; i < nc; ++i)
        {
            if (! ok[i]) continue;
            const Cable& c = d.c[i];
            const bool fromSend = c.from == kAmpA && c.fp == 1, toRet = c.to == kAmpA && c.tp == 1;
            const bool fromIn = p.inLoop[c.from], toIn = p.inLoop[c.to];
            if (fromSend || toRet || fromIn || toIn)
            {
                const bool a = fromSend || fromIn, b = toRet || toIn;
                if (a && b && p.hasLoop) isLoopCable[i] = true; else ok[i] = false;
            }
        }
        for (int i = 0; i < nc; ++i) p.cableOk[i] = ok[i];

        // ---- reachability for the main run
        bool fromIn[kNodes] = {}, toOut[kNodes] = {};
        fromIn[kIn] = true;  toOut[kOut] = true;
        for (bool again = true; again;)
        {
            again = false;
            for (int i = 0; i < nc; ++i)
                if (ok[i] && ! isLoopCable[i] && fromIn[d.c[i].from] && ! fromIn[d.c[i].to]) { fromIn[d.c[i].to] = true; again = true; }
        }
        for (bool again = true; again;)
        {
            again = false;
            for (int i = 0; i < nc; ++i)
                if (ok[i] && ! isLoopCable[i] && toOut[d.c[i].to] && ! toOut[d.c[i].from]) { toOut[d.c[i].from] = true; again = true; }
        }
        for (int n = 0; n < kNodes; ++n) p.active[n] = fromIn[n] && toOut[n] && ! p.inLoop[n];

        // ---- order (Kahn). A node left over has a cycle through it and is dropped.
        int indeg[kNodes] = {};
        bool used[kMaxCables] = {};
        for (int i = 0; i < nc; ++i)
            if (ok[i] && ! isLoopCable[i] && p.active[d.c[i].from] && p.active[d.c[i].to]) { used[i] = true; ++indeg[d.c[i].to]; }
        bool done[kNodes] = {};
        int order[kNodes], no = 0;
        for (bool again = true; again;)
        {
            again = false;
            for (int n = 0; n < kNodes; ++n)
            {
                if (! p.active[n] || done[n] || indeg[n] != 0) continue;
                done[n] = true;  order[no++] = n;  again = true;
                for (int i = 0; i < nc; ++i) if (used[i] && d.c[i].from == n) --indeg[d.c[i].to];
                break;                                           // restart the scan so the order stays stable (lowest number first among the ready ones)
            }
        }
        for (int n = 0; n < kNodes; ++n) if (p.active[n] && ! done[n]) p.active[n] = false;
        for (int i = 0; i < nc; ++i) if (used[i] && ! (p.active[d.c[i].from] && p.active[d.c[i].to])) { used[i] = false; ok[i] = false; p.cableOk[i] = false; }

        // ---- where the signal is stereo: everything downstream of the cab
        bool post[kNodes] = {};
        for (bool again = true; again;)
        {
            again = false;
            for (int i = 0; i < nc; ++i)
                if (used[i] && (d.c[i].from == kCab || post[d.c[i].from]) && ! post[d.c[i].to]) { post[d.c[i].to] = true; again = true; }
        }
        for (int n = 0; n < kNodes; ++n) p.group[n] = p.inLoop[n] ? 2 : (post[n] ? 1 : 0);

        auto makeIn = [&] (const Cable& c) -> In
        {
            In in;
            in.src = (uint8_t) c.from;  in.sp = (uint8_t) c.fp;  in.chan = c.chan;
            float lvl = c.levelDb <= kMuteDb ? 0.0f : dbToGain (c.levelDb);
            if (c.pol) lvl = -lvl;
            in.lvl = lvl;
            const bool stereo = p.group[c.to] == 1 || (c.to == kOut && p.group[c.from] == 1);
            if (stereo && c.pan != 0.0f) { in.usePan = true;  panGains (c.pan, in.panL, in.panR); }
            return in;
        };
        auto addIn = [&] (Step& s, const Cable& c, int port)
        {
            if (s.n[port] < kMaxIn) s.in[port][s.n[port]++] = makeIn (c);
        };

        for (int k = 0; k < no; ++k)
        {
            Step& s = p.steps[p.nSteps++];
            s.node = (uint8_t) order[k];
            for (int i = 0; i < nc; ++i)
                if (used[i] && d.c[i].to == order[k]) addIn (s, d.c[i], (order[k] == kCab && d.c[i].tp == 1) ? 1 : 0);
        }
        // Amp A's return jack, and the loop's own run order
        if (p.hasLoop)
        {
            bool ldone[kNodes] = {};
            // only edges between loop members are dependencies (the send jack is always ready)
            int dep[kNodes] = {};
            for (int i = 0; i < nc; ++i) if (isLoopCable[i] && p.inLoop[d.c[i].from] && p.inLoop[d.c[i].to]) ++dep[d.c[i].to];
            for (bool again = true; again;)
            {
                again = false;
                for (int n = 0; n < kNodes; ++n)
                {
                    if (! p.inLoop[n] || ldone[n] || dep[n] != 0) continue;
                    ldone[n] = true;  again = true;
                    Step& s = p.loop[p.nLoop++];
                    s.node = (uint8_t) n;
                    for (int i = 0; i < nc; ++i) if (isLoopCable[i] && d.c[i].to == n) addIn (s, d.c[i], 0);
                    for (int i = 0; i < nc; ++i) if (isLoopCable[i] && d.c[i].from == n && p.inLoop[d.c[i].to]) --dep[d.c[i].to];
                    break;
                }
            }
            for (int n = 0; n < kNodes; ++n)       // a loop member left over is in a cycle: it is not run
                if (p.inLoop[n] && ! ldone[n]) p.inLoop[n] = false;
            // the return jack
            for (int k = 0; k < p.nSteps; ++k)
                if (p.steps[k].node == kAmpA)
                    for (int i = 0; i < nc; ++i)
                        if (isLoopCable[i] && d.c[i].to == kAmpA && d.c[i].tp == 1 && (p.inLoop[d.c[i].from] || (d.c[i].from == kAmpA && d.c[i].fp == 1))) addIn (p.steps[k], d.c[i], 1);
            for (int n = 0; n < kNodes; ++n) if (p.inLoop[n]) { if (n == 14) p.tremInLoop = true; else p.loopHasBlocks = true; }
            if (p.nLoop == 0) p.hasLoop = false;
        }

        // ---- drives that are built into the amp: the ones right in front of it (nothing else between them and the amp, nothing else added in)
        auto unity = [&] (const Cable& c) { return c.levelDb == 0.0f && ! c.pol && c.pan == 0.0f && c.chan == 0; };
        auto inCount = [&] (int node, int port)
        {
            int k = 0;
            for (int i = 0; i < nc; ++i) if (used[i] && d.c[i].to == node && d.c[i].tp == port) ++k;
            return k;
        };
        bool tailOk[2] = { false, false };
        for (int pass = 0; pass < 2; ++pass)
            for (int dd = 0; dd < 2; ++dd)
            {
                const int blk = 22 + dd;
                if (! p.active[blk]) continue;
                bool any = false, all = true;
                for (int i = 0; i < nc && all; ++i)
                {
                    if (! used[i] || d.c[i].from != blk) continue;
                    any = true;
                    const Cable& c = d.c[i];
                    const bool toAmp = (c.to == kAmpA || c.to == kAmpB) && c.tp == 0 && inCount (c.to, 0) == 1;
                    const bool toDrive = (c.to == 22 || c.to == 23) && c.to != blk && tailOk[c.to - 22] && inCount (c.to, 0) == 1;
                    if (! (unity (c) && (toAmp || toDrive))) all = false;
                }
                tailOk[dd] = any && all;
            }
        // signal order of the tail drives: one that feeds the other comes first, otherwise Drive 1 first
        for (int dd = 0; dd < 2; ++dd) if (tailOk[dd]) p.tail[p.nTail++] = (uint8_t) (22 + dd);
        if (p.nTail == 2)
            for (int i = 0; i < nc; ++i)
                if (used[i] && d.c[i].from == 23 && d.c[i].to == 22) { p.tail[0] = 23; p.tail[1] = 22; }
    }

    // ---------------------------------------------------------------------------------------------------------------------
    // editing helpers (message thread)
    // ---------------------------------------------------------------------------------------------------------------------
    inline int findCable (const Data& d, int from, int fp, int to, int tp)
    {
        for (int i = 0; i < d.n; ++i)
            if (d.c[i].from == from && d.c[i].fp == fp && d.c[i].to == to && d.c[i].tp == tp) return i;
        return -1;
    }
    inline void removeCable (Data& d, int i)
    {
        if (i < 0 || i >= d.n) return;
        for (int k = i; k + 1 < d.n; ++k) d.c[k] = d.c[k + 1];
        --d.n;
    }
    // Amp A is two parts as far as loops go: its front (input, send) and its back (return, output). Vertex kNodes is the back.
    inline int vertexOut (int node, int port) { return (node == kAmpA && port == 0) ? kNodes : node; }
    inline int vertexIn  (int node, int port) { return (node == kAmpA && port == 1) ? kNodes : node; }
    inline bool reaches (const Data& d, int start, int target)
    {
        bool seen[kNodes + 1] = {};
        int stack[kNodes + 1], sp = 0;
        stack[sp++] = start;  seen[start] = true;
        while (sp > 0)
        {
            const int v = stack[--sp];
            if (v == target) return true;
            auto push = [&] (int w) { if (! seen[w]) { seen[w] = true; stack[sp++] = w; } };
            if (v == kAmpA) push (kNodes);                                            // front -> back, inside the amp
            for (int i = 0; i < d.n; ++i)
                if (vertexOut (d.c[i].from, d.c[i].fp) == v) push (vertexIn (d.c[i].to, d.c[i].tp));
        }
        return false;
    }
    inline bool wouldLoopBack (const Data& d, int from, int fp, int to, int tp)
    {
        return reaches (d, vertexIn (to, tp), vertexOut (from, fp));
    }
    // adds a cable; false if it makes no sense (bad ends, already there, would feed itself, no room)
    inline bool connect (Data& d, int from, int fp, int to, int tp, float levelDb = 0.0f, float pan = 0.0f, bool pol = false, int chan = 0)
    {
        Cable c;
        c.from = (int8_t) from;  c.fp = (int8_t) fp;  c.to = (int8_t) to;  c.tp = (int8_t) tp;
        c.levelDb = levelDb;  c.pan = pan;  c.pol = pol ? 1 : 0;  c.chan = (uint8_t) chan;
        if (! cableShapeOk (c) || d.n >= kMaxCables || findCable (d, from, fp, to, tp) >= 0) return false;
        if (wouldLoopBack (d, from, fp, to, tp)) return false;
        d.c[d.n++] = c;
        return true;
    }
    // puts a node in the middle of an existing cable: a -> b becomes a -> node -> b (the original level, pan and so on stay on the first half)
    inline bool insertOnCable (Data& d, int idx, int node)
    {
        if (idx < 0 || idx >= d.n || ! isNode (node) || node == kIn || node == kOut) return false;
        const Cable old = d.c[idx];
        if (old.from == node || old.to == node) return false;
        Data t = d;
        removeCable (t, idx);
        if (! connect (t, old.from, old.fp, node, 0, old.levelDb, old.pan, old.pol != 0, old.chan)) return false;
        if (! connect (t, node, 0, old.to, old.tp)) return false;
        d = t;
        return true;
    }
    // takes a node out and joins every cable that came in to every cable that left (levels add), so the signal keeps flowing
    inline void removeNodeBypass (Data& d, int node)
    {
        Cable ins[kMaxCables], outs[kMaxCables];  int ni = 0, no = 0;
        for (int i = 0; i < d.n; ++i)
        {
            if (d.c[i].to == node)   ins[ni++] = d.c[i];
            if (d.c[i].from == node) outs[no++] = d.c[i];
        }
        Data t = d;
        for (int i = t.n - 1; i >= 0; --i) if (t.c[i].from == node || t.c[i].to == node) removeCable (t, i);
        for (int a = 0; a < ni; ++a)
            for (int b = 0; b < no; ++b)
                connect (t, ins[a].from, ins[a].fp, outs[b].to, outs[b].tp, ins[a].levelDb + outs[b].levelDb, ins[a].pan != 0.0f ? ins[a].pan : outs[b].pan,
                         (ins[a].pol != 0) != (outs[b].pol != 0), outs[b].chan != 0 ? outs[b].chan : ins[a].chan);
        d = t;
    }
    // b56: a dry path around a box. Every box that feeds it gets a direct cable to every box it feeds, so what comes out of the box is mixed
    // with the signal that skipped it. (The new cables start at the combined level of the two they shortcut; select one to change the blend.)
    inline bool hasDryPath (const Data& d, int node)
    {
        int found = 0;
        for (int a = 0; a < d.n; ++a)
        {
            if (d.c[a].to != node) continue;
            for (int b = 0; b < d.n; ++b)
            {
                if (d.c[b].from != node) continue;
                if (findCable (d, d.c[a].from, d.c[a].fp, d.c[b].to, d.c[b].tp) < 0) return false;
                ++found;
            }
        }
        return found > 0;
    }
    inline bool addDryPath (Data& d, int node)
    {
        Cable ins[kMaxCables], outs[kMaxCables];  int ni = 0, no = 0;
        for (int i = 0; i < d.n; ++i)
        {
            if (d.c[i].to == node)   ins[ni++] = d.c[i];
            if (d.c[i].from == node) outs[no++] = d.c[i];
        }
        Data t = d;  bool any = false;
        for (int a = 0; a < ni; ++a)
            for (int b = 0; b < no; ++b)
                any = connect (t, ins[a].from, ins[a].fp, outs[b].to, outs[b].tp, ins[a].levelDb + outs[b].levelDb, ins[a].pan != 0.0f ? ins[a].pan : outs[b].pan,
                               (ins[a].pol != 0) != (outs[b].pol != 0), outs[b].chan != 0 ? outs[b].chan : ins[a].chan) || any;
        if (any) d = t;
        return any;
    }
    inline void removeDryPath (Data& d, int node)
    {
        Cable ins[kMaxCables], outs[kMaxCables];  int ni = 0, no = 0;
        for (int i = 0; i < d.n; ++i)
        {
            if (d.c[i].to == node)   ins[ni++] = d.c[i];
            if (d.c[i].from == node) outs[no++] = d.c[i];
        }
        for (int a = 0; a < ni; ++a)
            for (int b = 0; b < no; ++b)
                removeCable (d, findCable (d, ins[a].from, ins[a].fp, outs[b].to, outs[b].tp));
    }
    inline void disconnectNode (Data& d, int node)
    {
        for (int i = d.n - 1; i >= 0; --i) if (d.c[i].from == node || d.c[i].to == node) removeCable (d, i);
    }
    inline int cableCountOf (const Data& d, int node)
    {
        int k = 0;
        for (int i = 0; i < d.n; ++i) if (d.c[i].from == node || d.c[i].to == node) ++k;
        return k;
    }

    // ---- b46: moves the editor makes -----------------------------------------------------------------------------------
    // Nodes that can sit in the middle of a single cable: one way in, one way out (the pedals and NAM pedal 1).
    inline bool isInline (int n) { return isPedal (n) || n == kNamP1; }

    // true if the data has no cycle through anything but Amp A's own front-to-back path
    inline bool acyclic (const Data& d)
    {
        for (int i = 0; i < d.n; ++i)
            if (reaches (d, vertexIn (d.c[i].to, d.c[i].tp), vertexOut (d.c[i].from, d.c[i].fp)) && ! (d.c[i].from == d.c[i].to && d.c[i].from == kAmpA)) return false;
        return true;
    }

    // puts `node` right in front of a jack: whatever fed that jack now feeds the node, and the node feeds the jack.
    // (If it was somewhere else, it leaves there first and the signal flows on round it.) False if that would make a loop.
    inline bool insertBefore (Data& d, int node, int target, int tp = 0)
    {
        if (! isInline (node) || ! isNode (target) || target == node || target == kIn) return false;
        Data t = d;
        removeNodeBypass (t, node);
        if (target == kAmpA && tp == 1)
        {
            // the amp's return jack: with nothing in the loop yet, the node goes between the send and the return
            bool any = false;
            for (int i = 0; i < t.n; ++i) if (t.c[i].to == kAmpA && t.c[i].tp == 1) any = true;
            if (! any) { if (! connect (t, kAmpA, 1, node, 0) || ! connect (t, node, 0, kAmpA, 1)) return false;  d = t;  return true; }
        }
        Cable moved[kMaxCables];  int nm = 0;
        for (int i = t.n - 1; i >= 0; --i)
            if (t.c[i].to == target && t.c[i].tp == tp) { moved[nm++] = t.c[i];  removeCable (t, i); }
        for (int k = nm - 1; k >= 0; --k)                              // keep the original order
        {
            Cable c = moved[k];  c.to = (int8_t) node;  c.tp = 0;
            if (t.n >= kMaxCables) return false;
            t.c[t.n++] = c;
        }
        if (! connect (t, node, 0, target, tp)) return false;
        if (! acyclic (t)) return false;
        d = t;
        return true;
    }
    // puts `node` right after an output jack: whatever that jack fed is now fed by the node, and the jack feeds the node
    inline bool insertAfter (Data& d, int node, int source, int fp = 0)
    {
        if (! isInline (node) || ! isNode (source) || source == node || source == kOut) return false;
        Data t = d;
        removeNodeBypass (t, node);
        if (source == kAmpA && fp == 1)
        {
            bool any = false;
            for (int i = 0; i < t.n; ++i) if (t.c[i].from == kAmpA && t.c[i].fp == 1) any = true;
            if (! any) { if (! connect (t, kAmpA, 1, node, 0) || ! connect (t, node, 0, kAmpA, 1)) return false;  d = t;  return true; }
        }
        Cable moved[kMaxCables];  int nm = 0;
        for (int i = t.n - 1; i >= 0; --i)
            if (t.c[i].from == source && t.c[i].fp == fp) { moved[nm++] = t.c[i];  removeCable (t, i); }
        for (int k = nm - 1; k >= 0; --k)
        {
            Cable c = moved[k];  c.from = (int8_t) node;  c.fp = 0;
            if (t.n >= kMaxCables) return false;
            t.c[t.n++] = c;
        }
        if (! connect (t, source, fp, node, 0)) return false;
        if (! acyclic (t)) return false;
        d = t;
        return true;
    }
    // a pedal beside a cable instead of on it: a -> node -> b is added next to a -> b (the signal now takes both paths)
    inline bool addBranch (Data& d, int idx, int node)
    {
        if (idx < 0 || idx >= d.n || ! isInline (node)) return false;
        const Cable old = d.c[idx];
        if (old.from == node || old.to == node) return false;
        Data t = d;
        removeNodeBypass (t, node);
        // the cable index may have moved when the node came out; find the cable again
        const int j = findCable (t, old.from, old.fp, old.to, old.tp);
        if (j < 0) return false;
        if (! connect (t, old.from, old.fp, node, 0) || ! connect (t, node, 0, old.to, old.tp)) return false;
        if (! acyclic (t)) return false;
        d = t;
        return true;
    }
    // a split on a cable: a -> b becomes a -> split, with the two halves added back together at b.
    // A crossover sends the lows out of jack 0 and the highs out of jack 1; the left / right split sends the left side and the right side.
    inline bool insertSplit (Data& d, int idx, int splitNode)
    {
        if (idx < 0 || idx >= d.n || (! isXover (splitNode) && splitNode != kLr)) return false;
        if (cableCountOf (d, splitNode) > 0) return false;                       // this one is in use already
        const Cable old = d.c[idx];
        if (old.from == splitNode || old.to == splitNode) return false;
        Data t = d;
        removeCable (t, idx);
        if (! connect (t, old.from, old.fp, splitNode, 0, old.levelDb, old.pan, old.pol != 0, old.chan)) return false;
        const bool lr = splitNode == kLr;
        if (! connect (t, splitNode, 0, old.to, old.tp, 0.0f, 0.0f, false, lr ? 1 : 0)) return false;
        if (! connect (t, splitNode, 1, old.to, old.tp, 0.0f, 0.0f, false, lr ? 2 : 0)) return false;
        if (! acyclic (t)) return false;
        d = t;
        return true;
    }
    // puts a pedal in one of the three usual places: lane 0 before the amp (in front of NAM pedal 1 and the drives that feed it), lane 1 at the very end
    // (just before the output), lane 2 at the end of the amp's FX loop
    inline bool placeNear (Data& d, int node, int lane)
    {
        if (! isInline (node)) return false;
        if (lane == 2) return insertBefore (d, node, kAmpA, 1);
        if (lane == 1) return insertBefore (d, node, kOut, 0);
        int target = kAmpA;
        for (bool again = true; again && node != kNamP1;)
        {
            again = false;
            for (int i = 0; i < d.n; ++i)
                if (d.c[i].to == target && d.c[i].tp == 0 && (d.c[i].from == 22 || d.c[i].from == 23 || d.c[i].from == kNamP1) && d.c[i].from != node)
                { target = d.c[i].from;  again = true;  break; }
        }
        return insertBefore (d, node, target, 0);
    }
    // where a node sits now: 0 before the cab (mono), 1 after the cab, 2 inside the amp's loop; -1 if the signal does not pass it
    inline int laneOf (const Data& d, int node)
    {
        static Plan p;
        compile (d, p);
        if (node < 0 || node >= kNodes) return -1;
        if (p.inLoop[node]) return 2;
        return p.active[node] ? (int) p.group[node] : -1;
    }

    // a node and every cable on it leave the board; the signal flows on round it
    inline void takeOff (Data& d, int node)
    {
        if (isInline (node)) removeNodeBypass (d, node);
        else disconnectNode (d, node);
    }

    // ---------------------------------------------------------------------------------------------------------------------
    // text form (saved with the session)
    // ---------------------------------------------------------------------------------------------------------------------
    inline std::string toString (const Data& d)
    {
        std::string s = "P2";
        char b[160];
        std::snprintf (b, sizeof b, "|x:%g,%g,%g", (double) d.xoHz[0], (double) d.xoHz[1], (double) d.xoHz[2]);
        s += b;
        s += "|c:";
        for (int i = 0; i < d.n; ++i)
        {
            const Cable& c = d.c[i];
            std::snprintf (b, sizeof b, "%s%d.%d>%d.%d,%g,%g,%d,%d", i ? ";" : "", (int) c.from, (int) c.fp, (int) c.to, (int) c.tp,
                           (double) c.levelDb, (double) c.pan, (int) c.pol, (int) c.chan);
            s += b;
        }
        return s;
    }
    // false if the text is not a patch; the data is left empty then
    inline bool fromString (const std::string& s, Data& d)
    {
        d = Data();
        if (s.size() < 2 || s[0] != 'P' || (s[1] != '1' && s[1] != '2')) return false;
        const bool oldNumbers = s[1] == '1';               // saved before b55: the nodes from the NAM pedal on were one number lower
        const size_t px = s.find ("|x:"), pc = s.find ("|c:");
        if (px == std::string::npos || pc == std::string::npos) return false;
        {
            const char* q = s.c_str() + px + 3;
            for (int i = 0; i < 3; ++i)
            {
                char* e = nullptr;
                const float v = std::strtof (q, &e);
                if (e == q) break;
                d.xoHz[i] = v < 20.0f ? 20.0f : v > 20000.0f ? 20000.0f : v;
                q = e;  if (*q == ',') ++q;
            }
        }
        const char* q = s.c_str() + pc + 3;
        while (*q != 0 && d.n < kMaxCables)
        {
            char* e = nullptr;
            int v[4]; float lv = 0.0f, pn = 0.0f; int pol = 0, ch = 0;
            v[0] = (int) std::strtol (q, &e, 10);  if (e == q || *e != '.') break;  q = e + 1;
            v[1] = (int) std::strtol (q, &e, 10);  if (e == q || *e != '>') break;  q = e + 1;
            v[2] = (int) std::strtol (q, &e, 10);  if (e == q || *e != '.') break;  q = e + 1;
            v[3] = (int) std::strtol (q, &e, 10);  if (e == q) break;  q = e;
            if (*q == ',') { ++q; lv = std::strtof (q, &e); q = e; }
            if (*q == ',') { ++q; pn = std::strtof (q, &e); q = e; }
            if (*q == ',') { ++q; pol = (int) std::strtol (q, &e, 10); q = e; }
            if (*q == ',') { ++q; ch = (int) std::strtol (q, &e, 10); q = e; }
            Cable c;
            if (oldNumbers) { if (v[0] >= 27) ++v[0];  if (v[2] >= 27) ++v[2]; }
            c.from = (int8_t) v[0];  c.fp = (int8_t) v[1];  c.to = (int8_t) v[2];  c.tp = (int8_t) v[3];
            c.levelDb = lv;  c.pan = pn;  c.pol = pol != 0 ? 1 : 0;  c.chan = (uint8_t) (ch < 0 || ch > 2 ? 0 : ch);
            if (cableShapeOk (c) && findCable (d, c.from, c.fp, c.to, c.tp) < 0) d.c[d.n++] = c;
            if (*q == ';') ++q; else break;
        }
        return true;
    }

    // ---------------------------------------------------------------------------------------------------------------------
    // the old three-lane pedalboard as a patch (used for the default board, and to open sessions saved before b46)
    // ---------------------------------------------------------------------------------------------------------------------
    struct LegacyLanes
    {
        int   order[3][kPedals] = {};
        int   row[3][kPedals] = {};
        int   loc[kPedals] = {};
        bool  on[kPedals] = {};
        int   split[3] = {}, regStart[3] = {}, regEnd[3] = {};
        float xoverHz[3] = { 800, 800, 800 }, bLvlDb[3] = {}, bPan[3] = {}, bal[3] = { 0.5f, 0.5f, 0.5f };
        bool  bPol[3] = {};
        float lvlDb[kPedals] = {}, pan[kPedals] = {};
        bool  pol[kPedals] = {};
    };

    struct List { int v[kPedals] = {}; int n = 0; void push (int b) { if (n < kPedals) v[n++] = b; } };

    inline void fromLegacy (const LegacyLanes& L, Data& d)
    {
        d = Data();
        auto add = [&] (int from, int fp, int to, int tp, float lvl = 0.0f, float pan = 0.0f, bool pol = false, int chan = 0)
        {
            if (d.n >= kMaxCables) return;
            Cable c;
            c.from = (int8_t) from;  c.fp = (int8_t) fp;  c.to = (int8_t) to;  c.tp = (int8_t) tp;
            c.levelDb = lvl;  c.pan = pan;  c.pol = pol ? 1 : 0;  c.chan = (uint8_t) chan;
            d.c[d.n++] = c;
        };
        // which drives are built in: still in the before-the-amp lane, on the main row, with nothing else (that can move) after them
        bool tail[2] = { true, true };
        for (int dd = 0; dd < 2; ++dd)
        {
            const int blk = 22 + dd;
            if (L.loc[blk] != 0 || L.row[0][blk] != 0) { tail[dd] = false; continue; }
            bool after = false;
            for (int k = 0; k < kPedals; ++k)
            {
                const int o = L.order[0][k];
                if (o == blk) { after = true; continue; }
                if (after && o != 22 && o != 23 && L.loc[o] == 0 && isPedal (o)) { tail[dd] = false; break; }
            }
        }
        for (int g = 0; g < 3; ++g)
        {
            const int srcN = g == 0 ? (int) kIn : g == 1 ? (int) kCab : (int) kAmpA, srcP = g == 2 ? 1 : 0;
            const int dstN = g == 0 ? (int) kNamP1 : g == 1 ? (int) kOut : (int) kAmpA, dstP = g == 2 ? 1 : 0;
            auto member = [&] (int b) { return isPedal (b) && L.loc[b] == g && ! (g == 0 && b >= 22 && b <= 23 && tail[b - 22]); };
            auto located = [&] (int b) { return isPedal (b) && L.loc[b] == g && ! (g == 2 && b == 14) && L.on[b]; };
            int mode = L.split[g];
            if (mode == 3 && g != 1) mode = 1;
            int rs = 0, re = kPedals - 1;
            {
                int ps = -1, pe = -1;
                for (int k = 0; k < kPedals; ++k)
                {
                    if (L.regStart[g] > 0 && L.order[g][k] == L.regStart[g] - 1 && located (L.regStart[g] - 1)) ps = k;
                    if (L.regEnd[g]   > 0 && L.order[g][k] == L.regEnd[g]   - 1 && located (L.regEnd[g]   - 1)) pe = k;
                }
                if (ps >= 0) rs = ps;
                if (pe >= 0) re = pe;
                if (rs > re) { rs = 0; re = kPedals - 1; }
            }
            auto isLower = [&] (int b) { return L.row[g][b] != 0 && located (b); };
            bool anyLow = false, anyUp = false;
            for (int k = rs; k <= re; ++k)
            {
                const int b = L.order[g][k];
                if (! member (b)) continue;
                if (isLower (b)) anyLow = true; else if (located (b)) anyUp = true;
            }
            const bool engage = mode <= 1 ? anyLow : (anyLow || anyUp);

            List before, upper, lower, after;
            for (int k = 0; k < kPedals; ++k)
            {
                const int b = L.order[g][k];
                if (! member (b)) continue;
                if (! engage) { before.push (b); continue; }
                if (k < rs) before.push (b);
                else if (k > re) after.push (b);
                else if (isLower (b)) lower.push (b);
                else upper.push (b);
            }
            int curN = srcN, curP = srcP;
            auto chain = [&] (const List& v, int& cn, int& cp, int from = 0) { for (int i = from; i < v.n; ++i) { add (cn, cp, v.v[i], 0); cn = v.v[i]; cp = 0; } };
            if (! engage)
            {
                chain (before, curN, curP);
                if (g != 2 || before.n > 0) add (curN, curP, dstN, dstP);
            }
            else
            {
                chain (before, curN, curP);
                const float bal = L.bal[g] < 0.0f ? 0.0f : L.bal[g] > 1.0f ? 1.0f : L.bal[g];
                const float gMain = std::fmin (1.0f, 2.0f * (1.0f - bal)), gPar = std::fmin (1.0f, 2.0f * bal);
                auto dbOf = [] (float lin) { return lin <= 0.0f ? -100.0f : (lin == 1.0f ? 0.0f : 20.0f * std::log10 (lin)); };
                // where the paths come together: the first pedal after the region, or the lane's end
                const int yN = after.n > 0 ? after.v[0] : dstN, yP = after.n > 0 ? 0 : dstP;
                const int xN = curN, xP = curP;
                if (mode == 0)
                {
                    int cn = xN, cp = xP;
                    chain (upper, cn, cp);
                    add (cn, cp, yN, yP, dbOf (gMain));
                    for (int li = 0; li < lower.n; ++li)
                    {
                        const int b = lower.v[li];
                        add (xN, xP, b, 0);
                        add (b, 0, yN, yP, L.lvlDb[b] + dbOf (gPar), (g == 1) ? L.pan[b] : 0.0f, L.pol[b]);
                    }
                }
                else
                {
                    int aN = xN, aP = xP, bN = -1, bP = 0;
                    if (mode == 2)
                    {
                        const int xo = kXo1 + g;                                  // one crossover per lane
                        add (xN, xP, xo, 0);
                        aN = xo;  aP = 0;  bN = xo;  bP = 1;
                        d.xoHz[xo - kXo1] = L.xoverHz[g];
                    }
                    else if (mode == 3)
                    {
                        add (xN, xP, kLr, 0);
                        aN = kLr;  aP = 0;  bN = kLr;  bP = 1;
                    }
                    else { bN = xN;  bP = xP; }
                    chain (upper, aN, aP);
                    chain (lower, bN, bP);
                    add (aN, aP, yN, yP, dbOf (gMain), 0.0f, false, mode == 3 ? 1 : 0);
                    add (bN, bP, yN, yP, L.bLvlDb[g] + dbOf (gPar), (mode != 3 && g == 1) ? L.bPan[g] : 0.0f, L.bPol[g], mode == 3 ? 2 : 0);
                }
                curN = yN;  curP = yP;
                if (after.n > 0)
                {
                    curN = after.v[0];  curP = 0;
                    chain (after, curN, curP, 1);                       // the first of 'after' already has its inputs from the merge; the rest follow it
                    add (curN, curP, dstN, dstP);
                }
            }
        }
        // the lane ends and the fixed parts: NAM pedal 1, the built-in drives, the two amps and the cab
        int cn = kNamP1;
        for (int dd = 0; dd < 2; ++dd) if (tail[dd]) { add (cn, 0, 22 + dd, 0);  cn = 22 + dd; }
        add (cn, 0, kAmpA, 0);
        add (cn, 0, kAmpB, 0);
        add (kAmpA, 0, kCab, 0);
        add (kAmpB, 0, kCab, 1);
    }

    // the pedals that start before the amp (the others start after the cab); the editor uses this to place a pedal that has no cables yet
    inline bool homeBeforeAmp (int b)
    {
        static const int home0[] = { 0, 1, 2, 3, 4, 5, 6, 7, 16, 19, 20, 21, 22, 23, 24, 25, 26 };
        for (int h : home0) if (h == b) return true;
        return b == kNamP1;
    }

    // every node in the order the signal meets it, for the strip under the tabs: the ones before the amp, Amp A, what is in its loop,
    // the cab, then the ones after the cab. Nodes with no way through (not plugged in) come last.
    inline void stageOrder (const Data& d, std::vector<int>& out)
    {
        static Plan p;                               // (a Plan is big; this is only ever called on the message thread)
        compile (d, p);
        out.clear();
        auto isFree = [&] (int n) { return n != kIn && n != kOut; };
        for (int k = 0; k < p.nSteps; ++k) { const int n = p.steps[k].node; if (isFree (n) && p.group[n] == 0 && n != kAmpA && n != kCab && n != kAmpB) out.push_back (n); }
        out.push_back (kAmpA);
        for (int k = 0; k < p.nLoop; ++k) out.push_back (p.loop[k].node);
        out.push_back (kCab);
        for (int k = 0; k < p.nSteps; ++k) { const int n = p.steps[k].node; if (isFree (n) && p.group[n] == 1) out.push_back (n); }
        for (int n = 0; n < kPedals; ++n)
            if (isPedal (n) && ! p.active[n] && ! p.inLoop[n]) out.push_back (n);
    }

    inline void defaultPatch (Data& d)
    {
        static const int pre[kPedals]  = { 19, 0, 26, 1, 2, 16, 3, 4, 5, 6, 7, 8, 14, 9, 10, 11, 12, 15, 13, 18, 17, 20, 21, 24, 25, 22, 23, 27 };
        static const int post[kPedals] = { 8, 18, 27, 14, 9, 10, 11, 12, 15, 17, 13, 6, 7, 0, 26, 1, 2, 3, 4, 5, 16, 19, 20, 21, 24, 25, 22, 23 };
        LegacyLanes L;
        for (int k = 0; k < kPedals; ++k) { L.order[0][k] = pre[k];  L.order[1][k] = post[k];  L.order[2][k] = k; }
        for (int b = 0; b < kPedals; ++b) L.loc[b] = 1;
        for (int b = 0; b < kPedals; ++b) if (homeBeforeAmp (b)) L.loc[b] = 0;
        fromLegacy (L, d);
    }
}
