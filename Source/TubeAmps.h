#pragma once
// Circuit models of whole tube amplifiers, built on TubeNet.h.
//   LeadFiftyCircuit: a 50 W master-volume lead amp: two cascaded gain stages with a bright volume control, a third gain
//   stage, a cathode follower, the passive tone stack, a master volume, a long-tail phase inverter with the presence /
//   negative-feedback network, and a push-pull pair of power pentodes into an output transformer, with a sagging supply.
#include "TubeNet.h"

namespace Tube
{
    inline double audioTaperPos (double k) { k = std::min (1.0, std::max (0.0, k)); return (std::pow (10.0, 2.0 * k) - 1.0) / 99.0; }

    struct AmpControls
    {
        float gain = 0.5f;        // preamp volume, 0 to 1 (audio taper)
        float bass = 0.5f, mid = 0.5f, treble = 0.5f;
        float master = 0.5f;      // master volume, 0 to 1 (audio taper)
        float presence = 0.4f;
        float bias = 0.0f;        // -1 cold to +1 hot
        float sag = 0.5f;         // 0 to 1
        int   preTube = 0;        // 0 12AX7, 1 12AT7, 2 12AY7
        int   pwrTube = 0;        // 0 as built (EL34), 1 EL34, 2 6L6, 3 6V6, 4 EL84, 5 KT88
        bool  siliconRect = false;
    };

    class LeadFiftyCircuit
    {
    public:
        void prepare (double sampleRate)
        {
            fs = sampleRate;
            triEls.clear();
            buildTables (0, 0);
            build();
            setTol (2);
            applyControls (ctl, true);
            settle();
        }
        void setTol (int m) { const double t = m == 0 ? 5.0e-6 : m == 1 ? 3.0e-4 : m == 2 ? 3.0e-3 : 2.0e-2; netA.setTolerance (t); netB.setTolerance (t); netC.setTolerance (t); netD.setTolerance (t); }
        void reset() { settle(); }

        // Called once per block.
        void configure (const AmpControls& c)
        {
            if (c.preTube != curPre || c.pwrTube != curPwr) { buildTables (c.preTube, c.pwrTube); }
            applyControls (c, false);
        }

        // One sample. Input in volts at the guitar jack; returns the speaker voltage (8 ohm tap).
        inline double process (double vin)
        {
            netA.setForced (fIn, vin);
            netA.setForced (fK2, netB.v (nK2));
            netA.solve();
            netB.setForced (fW, netA.v (nWp));
            netB.solve();
            netC.setForced (fCfg, netB.v (nP3));
            netC.setForced (fK5c, netD.v (nK5));
            netC.setForced (fN16c, netD.v (nN16));
            netC.solve();
            netD.setForced (fGa, netC.v (nGa));
            netD.setForced (fFb, vsec);
            netD.solve();
            const double out = power (netD.v (nDa), netD.v (nEa));
            if (! std::isfinite (out) || std::fabs (out) > 400.0) { settle(); return 0.0; }
            return out;
        }

        double lastSecondary() const { return vsec; }
        static constexpr double kPeakFull = 28.3;     // 50 W into 8 ohms, peak volts

    private:
        double fs = 48000.0;
        AmpControls ctl;
        int curPre = -1, curPwr = -1;
        TubeTable triTabs[3], pwrTabs[5];
        const TubeTable* triTab = nullptr;
        const TubeTable* pwrTab = nullptr;
        std::vector<std::pair<Net*, int>> triEls;
        double pwrVg2Ref = 365.0;

        // ---- network A: V1b, preamp volume with bright network, and V1a's grid ----
        Net netA;  int nG1, nP1, nK1, nX, nWp, nBn;
        int fIn, fB5a, fK2;  int rHi, rLo;
        // ---- network B: V1a, interstage divider, V2a ----
        Net netB;  int nP2, nK2, nQ, nG3, nP3, nK3;  int fW, fB5b, fB6a;
        // ---- network C: cathode follower, tone stack, master volume, phase inverter grid ----
        Net netC;  int nK4, nS, nT, nM, nN, nWm, nO, nBm, nGa;  int fCfg, fB6b, fK5c, fN16c;
        int rT1, rT2, rB, rM1, rM2, rMv1, rMv2;
        // ---- network D: phase inverter and power-tube grids ----
        Net netD;  int nGb, nP5a, nP5b, nK5, nN16, nN17, nNp, nDa, nEa;  int fGa, fBpi, fFb, fBias;  int rPres;

        // ---- power stage state ----
        double i1 = 0.05, i2 = 0.05, imag = 0.0, uPrev = 0.0, iAvg = 0.1, vsec = 0.0, bsDyn = 365.0;
        double lpState = 0.0;
        double rsag = 100.0, idleI = 0.11;
        double vbias = -30.0;

        bool tablesBuilt = false;
        void buildTables (int pre, int pwr)
        {
            if (! tablesBuilt)
            {
                triTabs[0].buildTriode (tube12AX7());  triTabs[1].buildTriode (tube12AT7());  triTabs[2].buildTriode (tube12AY7());
                const KorenPentode pts[5] = { tubeEL34(), tube6L6(), tube6V6(), tubeEL84(), tubeKT88() };
                for (int i = 0; i < 5; ++i) { pwrTabs[i].setGridRange (-80.0, 30.0, 0.25); pwrTabs[i].buildPentode (pts[i], pwrVg2Ref); }
                tablesBuilt = true;
            }
            curPre = pre;  curPwr = pwr;
            triTab = &triTabs[pre == 1 ? 1 : pre == 2 ? 2 : 0];
            pwrTab = &pwrTabs[pwr == 2 ? 1 : pwr == 3 ? 2 : pwr == 4 ? 3 : pwr == 5 ? 4 : 0];
            for (auto& e : triEls) e.first->setTable (e.second, triTab);
        }

        static constexpr double kGridIs = 3.0e-8, kGridVt = 0.2;
        static constexpr double kPwrGridIs = 2.0e-6, kPwrGridVt = 0.35;

        void build()
        {
            // --- A ---
            nG1 = netA.node(); nP1 = netA.node(); nK1 = netA.node(); nX = netA.node(); nWp = netA.node(); nBn = netA.node();
            fIn = netA.forced(); fB5a = netA.forced(); fK2 = netA.forced();
            netA.R (fIn, nG1, 68.0e3);  netA.R (nG1, Net::GND, 1.0e6);
            netA.diode (nG1, nK1, kGridIs, kGridVt);
            netA.R (fB5a, nP1, 100.0e3);  triEls.push_back ({ &netA, netA.triode (nG1, nP1, nK1, triTab) });
            netA.R (nK1, Net::GND, 2700.0);  netA.C (nK1, Net::GND, 0.68e-6);
            netA.C (nP1, nX, 0.022e-6);
            rHi = netA.Rvar (nX, nWp, 500.0e3);  rLo = netA.Rvar (nWp, Net::GND, 500.0e3);
            netA.R (nX, nBn, 470.0e3);  netA.C (nX, nBn, 470.0e-12);  netA.C (nBn, nWp, 1.0e-9);
            netA.diode (nWp, fK2, kGridIs, kGridVt);
            netA.prepare (fs);  netA.setTolerance (3.0e-4);  netA.setChord (true);
            netA.setForced (fB5a, 256.0);  netA.setForced (fK2, 2.4);

            // --- B ---
            nP2 = netB.node(); nK2 = netB.node(); nQ = netB.node(); nG3 = netB.node(); nP3 = netB.node(); nK3 = netB.node();
            fW = netB.forced(); fB5b = netB.forced(); fB6a = netB.forced();
            netB.R (fB5b, nP2, 100.0e3);  triEls.push_back ({ &netB, netB.triode (fW, nP2, nK2, triTab) });
            netB.R (nK2, Net::GND, 10.0e3);
            netB.C (nP2, nQ, 0.022e-6);
            netB.R (nQ, nG3, 470.0e3);  netB.C (nQ, nG3, 470.0e-12);  netB.R (nG3, Net::GND, 470.0e3);
            netB.diode (nG3, nK3, kGridIs, kGridVt);
            netB.R (fB6a, nP3, 100.0e3);  triEls.push_back ({ &netB, netB.triode (nG3, nP3, nK3, triTab) });
            netB.R (nK3, Net::GND, 820.0);
            netB.prepare (fs);  netB.setTolerance (3.0e-4);  netB.setChord (true);
            netB.setForced (fB5b, 256.0);  netB.setForced (fB6a, 264.0);  netB.setForced (fW, 0.0);

            // --- C ---
            nK4 = netC.node(); nS = netC.node(); nT = netC.node(); nM = netC.node(); nN = netC.node(); nWm = netC.node();
            nO = netC.node(); nBm = netC.node(); nGa = netC.node();
            fCfg = netC.forced(); fB6b = netC.forced(); fK5c = netC.forced(); fN16c = netC.forced();
            triEls.push_back ({ &netC, netC.triode (fCfg, fB6b, nK4, triTab) });
            netC.R (nK4, Net::GND, 100.0e3);
            netC.C (nK4, nT, 470.0e-12);  netC.R (nK4, nS, 33.0e3);
            netC.C (nS, nM, 0.022e-6);  netC.C (nS, nWm, 0.022e-6);
            rT1 = netC.Rvar (nT, nO, 125.0e3);  rT2 = netC.Rvar (nO, nM, 125.0e3);
            rB  = netC.Rvar (nM, nN, 500.0e3);
            rM1 = netC.Rvar (nN, nWm, 12.5e3);  rM2 = netC.Rvar (nWm, Net::GND, 12.5e3);
            rMv1 = netC.Rvar (nO, nBm, 500.0e3);  rMv2 = netC.Rvar (nBm, Net::GND, 500.0e3);
            netC.C (nBm, nGa, 0.022e-6);  netC.R (nGa, fN16c, 1.0e6);
            netC.diode (nGa, fK5c, kGridIs, kGridVt);
            netC.prepare (fs);  netC.setTolerance (3.0e-4);  netC.setChord (true);
            netC.setForced (fB6b, 264.0);  netC.setForced (fCfg, 150.0);  netC.setForced (fK5c, 37.5);  netC.setForced (fN16c, 36.5);

            // --- D ---
            nGb = netD.node(); nP5a = netD.node(); nP5b = netD.node(); nK5 = netD.node(); nN16 = netD.node(); nN17 = netD.node();
            nNp = netD.node(); nDa = netD.node(); nEa = netD.node();
            fGa = netD.forced(); fBpi = netD.forced(); fFb = netD.forced(); fBias = netD.forced();
            triEls.push_back ({ &netD, netD.triode (fGa, nP5a, nK5, triTab) });  triEls.push_back ({ &netD, netD.triode (nGb, nP5b, nK5, triTab) });
            netD.R (fBpi, nP5a, 82.0e3);  netD.R (fBpi, nP5b, 100.0e3);
            netD.R (nK5, nN16, 470.0);  netD.R (nN16, nN17, 10.0e3);  netD.R (nN17, Net::GND, 4700.0);
            netD.R (nGb, nN16, 1.0e6);  netD.C (nGb, nN17, 0.1e-6);
            netD.R (fFb, nN17, 100.0e3);
            netD.C (nN17, nNp, 0.1e-6);  rPres = netD.Rvar (nNp, Net::GND, 10.0e3);
            netD.C (nP5a, nDa, 0.022e-6);  netD.R (nDa, fBias, 220.0e3);  netD.diode (nDa, Net::GND, kPwrGridIs, kPwrGridVt);
            netD.C (nP5b, nEa, 0.022e-6);  netD.R (nEa, fBias, 220.0e3);  netD.diode (nEa, Net::GND, kPwrGridIs, kPwrGridVt);
            netD.diode (nGb, nK5, kGridIs, kGridVt);
            netD.prepare (fs);  netD.setTolerance (3.0e-4);  netD.setChord (true);
            netD.setForced (fBpi, 300.0);  netD.setForced (fGa, 36.5);  netD.setForced (fFb, 0.0);  netD.setForced (fBias, -30.0);
        }

        void applyControls (const AmpControls& c, bool force)
        {
            ctl = c;
            const double fv = std::max (1.0e-3, audioTaperPos (c.gain));
            netA.setR (rLo, std::max (500.0, fv * 1.0e6));  netA.setR (rHi, std::max (500.0, (1.0 - fv) * 1.0e6));
            const double t = std::min (0.999, std::max (0.001, (double) c.treble)), b = std::min (0.999, std::max (0.001, (double) c.bass)),
                         m = std::min (0.999, std::max (0.001, (double) c.mid));
            netC.setR (rT1, std::max (100.0, (1.0 - t) * 250.0e3));  netC.setR (rT2, std::max (100.0, t * 250.0e3));
            netC.setR (rB, std::max (100.0, b * 1.0e6));
            netC.setR (rM1, std::max (10.0, (1.0 - m) * 25.0e3));  netC.setR (rM2, std::max (10.0, m * 25.0e3));
            const double fm = std::max (2.0e-3, audioTaperPos (c.master));
            netC.setR (rMv2, std::max (500.0, fm * 1.0e6));  netC.setR (rMv1, std::max (500.0, (1.0 - fm) * 1.0e6));
            netD.setR (rPres, std::max (20.0, (1.0 - (double) c.presence) * 22.0e3));
            vbias = -30.0 + 6.0 * (double) c.bias;
            netD.setForced (fBias, vbias);
            rsag = (c.siliconRect ? 25.0 : 140.0) * (0.3 + 1.4 * (double) c.sag);
            (void) force;
        }

        void settle()
        {
            // The operating point, passing the coupling between the networks a few times
            double k2 = 2.4, p3 = 150.0, k5 = 37.5, n16 = 36.5, ga = 36.5;
            for (int pass = 0; pass < 4; ++pass)
            {
                netA.setForced (fIn, 0.0);  netA.setForced (fK2, k2);
                netA.setGuess (nG1, 0.0); netA.setGuess (nP1, 190.0); netA.setGuess (nK1, 1.6); netA.setGuess (nWp, 0.0);
                netA.dcSolve();
                netB.setForced (fW, netA.v (nWp));
                netB.setGuess (nP2, 230.0); netB.setGuess (nK2, 2.4); netB.setGuess (nP3, 150.0); netB.setGuess (nK3, 0.9);
                netB.dcSolve();
                k2 = netB.v (nK2);  p3 = netB.v (nP3);
                netC.setForced (fCfg, p3);  netC.setForced (fK5c, k5);  netC.setForced (fN16c, n16);
                netC.setGuess (nK4, p3 - 8.0);  netC.setGuess (nGa, ga);
                netC.dcSolve();
                ga = netC.v (nGa);
                netD.setForced (fGa, ga);  netD.setForced (fFb, 0.0);
                netD.setGuess (nGb, 36.5); netD.setGuess (nP5a, 185.0); netD.setGuess (nP5b, 175.0); netD.setGuess (nK5, 37.5);
                netD.setGuess (nN16, 36.5); netD.setGuess (nN17, 11.0); netD.setGuess (nDa, vbias); netD.setGuess (nEa, vbias);
                netD.dcSolve();
                k5 = netD.v (nK5);  n16 = netD.v (nN16);
            }
            // power stage at rest
            i1 = i2 = 0.05;  imag = 0.0;  uPrev = 0.0;  bsDyn = 365.0;  iAvg = idleI;  vsec = 0.0;  lpState = 0.0;
            for (int i = 0; i < 200; ++i) power (netD.v (nDa), netD.v (nEa));
        }

        // One sample of the push-pull output stage with the output transformer.
        double power (double vg1, double vg2)
        {
            const double bs0 = 365.0, rdc = 40.0, rpp = 3400.0, rpa = rpp / 4.0, lm = 25.0, lh = lm / 4.0;
            const double T = 1.0 / fs;
            const double k = T / (2.0 * lh);
            const double rpaP = rpa / (1.0 + rpa * k);
            const double ihist = imag + k * uPrev;
            // supply droop: the reservoir averages the current drawn
            const double bs = bs0 - rsag * (iAvg - idleI);
            bsDyn = bs;
            double a1 = i1, a2 = i2;
            double u = 0.0;
            for (int it = 0; it < 8; ++it)
            {
                const double di = a1 - a2;
                u = rpaP * (di - ihist);
                const double vp1 = bs - rdc * a1 - u, vp2 = bs - rdc * a2 + u;
                double ip1, gm1, gp1, ip2, gm2, gp2;
                evalPwr (vg1, vp1, bs - 120.0 * a1, ip1, gm1, gp1);
                evalPwr (vg2, vp2, bs - 120.0 * a2, ip2, gm2, gp2);
                const double f1 = a1 - ip1, f2 = a2 - ip2;
                const double j11 = 1.0 + gp1 * (rdc + rpaP), j12 = -gp1 * rpaP, j21 = -gp2 * rpaP, j22 = 1.0 + gp2 * (rdc + rpaP);
                const double det = j11 * j22 - j12 * j21;
                if (std::fabs (det) < 1.0e-12) break;
                double d1 = (-f1 * j22 + f2 * j12) / det, d2 = (-f2 * j11 + f1 * j21) / det;
                const double lim = 0.25;
                d1 = std::max (-lim, std::min (lim, d1));  d2 = std::max (-lim, std::min (lim, d2));
                a1 += d1;  a2 += d2;
                a1 = std::max (a1, 0.0);  a2 = std::max (a2, 0.0);
                if (std::fabs (d1) < 1.0e-7 && std::fabs (d2) < 1.0e-7) break;
            }
            i1 = a1;  i2 = a2;
            const double di = i1 - i2;
            u = rpaP * (di - ihist);
            imag = ihist + k * u;  uPrev = u;
            // supply average (about 30 ms)
            const double al = 1.0 - std::exp (-T / 0.030);
            iAvg += al * ((i1 + i2) * 1.08 - iAvg);
            // secondary voltage at the 8 ohm tap
            const double n = std::sqrt (rpp / 8.0);
            const double raw = 2.0 * u / n;
            // leakage inductance: a low-pass at about 14 kHz
            const double a = 1.0 - std::exp (-2.0 * 3.14159265358979 * 14000.0 * T);
            lpState += a * (raw - lpState);
            vsec = lpState;
            return vsec;
        }
        void evalPwr (double vgk, double vpk, double vg2, double& ip, double& gm, double& gp) const
        {
            const double r = std::max (vg2, 50.0) / pwrVg2Ref;
            double i, g, p;
            pwrTab->eval (vgk / r, vpk, i, g, p);
            const double s = r * std::sqrt (r);
            ip = std::max (0.0, i * s);  gm = g * s / r;  gp = p * s;
        }
    };
}
