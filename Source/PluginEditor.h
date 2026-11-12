#pragma once

#include "PluginProcessor.h"
#include "ToneMatch.h"
#include "ToneLab.h"
#include <map>
#include <string>

// Hover help for a parameter: written text if we have it, otherwise the parameter's own name.
juce::String tipFor (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId);

//==============================================================================
// Colours and sizes. Change the palette here to re-skin the whole plugin.
// The UI is laid out at a fixed "design size" (kWidth x kHeight) and scaled to
// fit the window, so resizing keeps every control in proportion.
//==============================================================================
namespace ui
{
    // Graphite and bone. Only the accent changes: it follows the amp you are playing (see StarterAmpEditor::applyAccent).
    const juce::Colour background { 0xff09090c };
    const juce::Colour panel      { 0xff14141a };
    const juce::Colour panelEdge  { 0xff2b2b35 };
    const juce::Colour text       { 0xffe9e6df };
    const juce::Colour textDim    { 0xff8b8896 };
    inline juce::Colour accent    { 0xffff8a63 };   // follows the amp model
    inline juce::Colour accent2   { 0xffffb08f };   // a lighter shade of the accent
    const juce::Colour knobBody   { 0xff1d1d26 };
    const juce::Colour track      { 0xff34343f };
    const juce::Colour colourB    { 0xff5ce1e6 };   // second trace in displays (the first uses the accent)
    const juce::Colour colourC    { 0xffc77dff };   // third
    const juce::Colour colourD    { 0xffb6f06b };   // fourth

    // Window: header (0-44), page tabs (46-74), signal-flow strip (80-110), then the page.
    constexpr int kWidth = 1500, kHeight = 614, kPageY = 84, kPageHeight = 520;
}

//==============================================================================
// Look and feel: the "haze" theme (see PluginEditor.cpp).
//==============================================================================
class AmpLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AmpLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    static juce::String comboCaption (const juce::ComboBox&);       // a dim label drawn inside a menu whose text does not say what it is for
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    // Re-applies the colours that depend on the accent (menus, tooltips).
    void applyAccent();
};

//==============================================================================
// A rotary knob with a name above it and the value below it, attached to one
// parameter. Double-click resets to the default. Click the value to type one.
//==============================================================================
class KnobControl : public juce::Component
{
public:
    KnobControl (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                 const juce::String& name, const juce::String& suffix, int decimals);

    void resized() override;
    const juce::String& getParamId() const { return paramId; }

    // Greys the knob out when the current settings make it do nothing, and says why in its tooltip.
    void setUsed (bool used, const juce::String& why);
    void setName (const char* n) { if (nameLabel.getText() != n) nameLabel.setText (n, juce::dontSendNotification); }   // b38: lets a knob be relabelled (Presence / Cut)
    void setTempoLocked (bool locked) { tempoLocked = locked; setEnabled (usedNow && ! tempoLocked); }   // a note value is chosen: the tempo sets this

private:
    juce::String paramId, baseTip, whyNow;
    bool usedNow = true, tempoLocked = false;
    juce::Label nameLabel;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

//==============================================================================
// What a panel looks like. Rack = a brushed-metal rack unit with ears and screws, Pedal = a coloured
// stompbox enclosure, Amp = black tolex with gold piping and a nameplate, Cab = speaker-cabinet cloth
// and piping, Wood = a wooden acoustic-amp front. Knobs follow the style of the panel they are in.
//==============================================================================
enum class PanelKind { Rack, Pedal, Amp, Cab, Wood };

//==============================================================================
// One section of the window: a title, a few rows of buttons/menus, then a row
// of knobs. The panel doesn't own its controls, the editor does.
//==============================================================================
class Panel : public juce::Component
{
public:
    explicit Panel (const juce::String& panelTitle) : title (panelTitle) {}

    // A row of controls sharing the width (optionally with relative weights).
    // A nullptr leaves an empty slot. 'page' = -1 means always shown; 0, 1, 2...
    // means shown only when the panel is on that page (see setPage).
    void addRow (std::initializer_list<juce::Component*> comps, std::vector<float> weights = {},
                 int height = 26, int page = -1);
    void addKnob (KnobControl& knob, int page = -1);

    // If set, the knobs wrap into several rows with this many columns.
    void setKnobColumns (int columns) { knobColumns = columns; }

    // Pick the hardware look. 'tint' is the enclosure colour of a Pedal.
    void setStyle (PanelKind newKind, juce::Colour newTint = juce::Colour (0xff3a4a5c))
    {
        kind = newKind;
        tint = newTint;
        cache = juce::Image();
        repaint();
    }
    PanelKind getKind() const { return kind; }
    const juce::String& getTitle() const { return title; }     // v73: the tidy-view tiles use these
    juce::Colour getTint() const { return tint; }

    // Which amp's look an Amp / Cab / Wood panel wears (0 to 4 = the five electric amps; see ampThemeFor).
    void setTheme (int newTheme)
    {
        if (newTheme == theme)
            return;
        theme = newTheme;
        cache = juce::Image();
        repaint();
    }
    int getTheme() const { return theme; }

    // Change the title text (used by the amp faceplate, which shows the model name) and redraw.
    void setTitle (const juce::String& newTitle)
    {
        if (newTitle == title)
            return;
        title = newTitle;
        cache = juce::Image();
        repaint();
    }
    // Cab / Amp look only: show the title as a big centred nameplate instead of the small badge.
    void setBigNameplate (bool big) { bigNameplate = big; cache = juce::Image(); repaint(); }
    // Throw away the cached faceplate (the accent changed).
    void refreshFace() { cache = juce::Image(); repaint(); }

    // Hide or show one control (a knob, or a component in a row) without removing it. Hidden knobs
    // are skipped when the knobs are laid out, so the others spread out to fill the space.
    void setHidden (juce::Component* c, bool hide);

    // Panels with tabs show only the rows and knobs of the current page.
    void setPage (int newPage);

    // Fades every control of the panel except 'keep' (normally its ON switch) while the effect is off.
    // The controls still work, so a setting can be dialled in before the effect is switched on.
    void setContentDimmed (juce::Component* keep, bool dim);
    bool isContentDimmed() const { return dimmed; }

    // BASIC / ADVANCED: controls marked advanced (a knob, or a component in a row) are shown only while the
    // panel's ADV button is on. The first call adds the button to the panel's title bar. A row whose controls
    // are all advanced disappears (and gives its space back) while ADV is off.
    void markAdvanced (juce::Component* c);
    bool hasAdvanced() const { return advBtnAdded; }
    void enableAdvButton();                 // b63: a panel whose ADV button only reveals a second panel (no advanced controls of its own)
    bool isAdvancedShown() const { return advancedShown; }
    void setAdvancedAvailable (bool available) { if (available != advAvailable) { advAvailable = available; resized(); } }
    void setAdvancedShown (bool shown);
    std::function<void (bool)> onAdvancedChanged;

    // Per-block presets: a small PRE button in the title bar. The editor decides what the menu holds.
    void setPresetHandler (std::function<void (juce::Component&)> handler);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Row { std::vector<juce::Component*> comps; std::vector<float> weights; int height; int page; };
    struct KnobItem { KnobControl* knob; int page; };

    juce::String title;
    std::vector<Row> rows;
    std::vector<KnobItem> knobs;
    std::vector<juce::Component*> hiddenComps;
    bool bigNameplate = false;
    std::vector<juce::Component*> advancedComps;
    juce::TextButton advButton { "ADV" };
    juce::TextButton presetButton { "PRE" };
    std::function<void (juce::Component&)> onPreset;
    bool advancedShown = false, advAvailable = true, advBtnAdded = false;
    void ensureAdvButton();
    bool isExplicitlyHidden (const juce::Component* c) const { return std::find (hiddenComps.begin(), hiddenComps.end(), c) != hiddenComps.end(); }
    bool isAdvancedHidden (const juce::Component* c) const
    {
        return ! advancedShown && std::find (advancedComps.begin(), advancedComps.end(), c) != advancedComps.end();
    }
    bool isHidden (const juce::Component* c) const { return isExplicitlyHidden (c) || isAdvancedHidden (c); }
    int knobColumns = 0, currentPage = 0;
    bool dimmed = false;

    // The faceplate is drawn once into an image (at the screen's pixel density) and reused.
    void renderFace (juce::Graphics&, int width, int height) const;
    PanelKind kind = PanelKind::Rack;
    int theme = 2;
    juce::Colour tint { 0xff3a4a5c };
    juce::Image cache;
};

//==============================================================================
// The signal-flow strip under the tabs: every stage in signal order, each with a bypass light.
// Click the light to switch the stage on or off, click the name to open its page.
//==============================================================================
class ChainStrip : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    struct Item { juce::String label, onId; int page; int group = -1, block = -1; juce::String onId2; };   // onId2: a second switch (the light is lit if either is on; a click switches both off, or the first on)

    ChainStrip (juce::AudioProcessorValueTreeState&, std::function<void (int)> openPage);
    ~ChainStrip() override { stopTimer(); }

    void setCurrentPage (int page) { currentPage = page; repaint(); }
    void setProcessor (StarterAmpProcessor* p) { processor = p; rebuildShown(); }
    void setModelName (const juce::String& name) { if (name != modelName) { modelName = name; repaint(); } }

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;       // v73: drag a stage to change the effect order
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }

private:
    juce::Rectangle<float> itemBounds (int index) const;
    // the second lane: branches that run beside a stage of the main row (a second fuzz, a second amp)
    struct Branch { juce::String label, onId, routeId, anchor; int page; };
    std::vector<Branch> branches;
    juce::Rectangle<float> branchBounds (int index) const;
    bool branchOn (int index) const;
    bool branchParallel (int index) const;
    bool isOn (int index) const;
    void rebuildShown();
    const Item& at (int position) const { return items[(size_t) shown[(size_t) position]]; }
    StarterAmpProcessor* processor = nullptr;
    std::vector<int> shown;       // which item is drawn at each position (the effects can be reordered)
    std::vector<int> stageNodes;  // b46: the patch's stage order the strip was last built from
    int visibleCount = 0;         // b35: how many of them are drawn (the old pedals that are off are not)
    void timerCallback() override;

    juce::AudioProcessorValueTreeState& apvts;
    std::function<void (int)> openPage;
    std::vector<Item> items;
    std::vector<char> lastState;
    int currentPage = 0, hover = -1;
    int pressed = -1;                  // the position under the mouse button (v73)
    bool dragging = false;
    juce::Point<float> downPos;
    juce::String modelName;
};

//==============================================================================
// The IR display: IR A and IR B's first few milliseconds, and the frequency
// response of the cab they make together, with the current delay, polarity,
// level and mix applied. Use it to see (and hear) what the alignment did.
//==============================================================================
class IRView : public Panel, private juce::Timer
{
public:
    explicit IRView (StarterAmpProcessor&);
    ~IRView() override { stopTimer(); }

    void paint (juce::Graphics&) override;

private:
    static constexpr int kPoints = 120;       // points on the response curves
    static constexpr int kWaveSamples = 288;  // 6 ms at 48 kHz
    static constexpr int kFftOrder = 13;      // 8192 points

    void timerCallback() override;
    void recompute (const std::vector<float>& values);
    std::vector<float> currentValues() const;
    void computeResponse (const std::vector<float>& ir, std::array<float, kPoints>& out);

    StarterAmpProcessor& proc;
    juce::dsp::FFT fft { kFftOrder };

    std::vector<float> lastValues;            // what we last drew (to notice changes)
    std::vector<float> irs[kCabBlocks];       // snapshots of the IRs
    juce::String report;
    bool active[kCabBlocks] {};               // block is on and has an IR
    int numActive = 0;

    std::array<float, kWaveSamples> waves[kCabBlocks] {};
    std::array<float, kPoints> resp[kCabBlocks] {}, respMix {};
};

//==============================================================================
// Compressor gain-reduction meter: a bar that fills from the left as the
// compressor turns the signal down (0 to 20 dB).
//==============================================================================
class GainReductionMeter : public juce::Component, private juce::Timer
{
public:
    explicit GainReductionMeter (StarterAmpProcessor& p) : proc (p) { startTimerHz (20); }
    ~GainReductionMeter() override { stopTimer(); }

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    StarterAmpProcessor& proc;
    float shown = 0.0f;
};

//==============================================================================
// One vertical fader of the Post EQ: frequency label above, dB value below.
//==============================================================================
class FaderControl : public juce::Component
{
public:
    FaderControl (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                  const juce::String& name);
    void resized() override;

private:
    juce::Label nameLabel;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

//==============================================================================
// The Post EQ's response curve (all nine bands added together). Redrawn only
// when a band, the mode or the Q changes.
//==============================================================================
//==============================================================================
// Input and output level meters (peak, with a short hold and a clip light; click to clear the light).
//==============================================================================
class MeterView : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    explicit MeterView (StarterAmpProcessor& p) : proc (p) { startTimerHz (30); }
    ~MeterView() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { clip[0] = clip[1] = false; repaint(); }

private:
    void timerCallback() override;
    StarterAmpProcessor& proc;
    float level[2] {}, hold[2] {};
    int holdAge[2] {};
    bool clip[2] { false, false };
};

//==============================================================================
// Tuner. Opens over the page; the output is muted while it is open. Two views:
//  - Single note: a chromatic tuner. Pitch is found with the YIN method on the last few thousand input samples.
//  - All strings: strum the chord and every string of the chosen tuning is read at once (PolyTuner.h). The tuning is
//    a preset or your own: 1 to 8 strings, a note for each string, and a whole-set shift. All of it is saved with the session.
//==============================================================================
class TunerView : public Panel, private juce::Timer
{
public:
    explicit TunerView (StarterAmpProcessor& p);
    ~TunerView() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void visibilityChanged() override;
    void resized() override;

    // Returns the pitch in Hz, or 0 if there is no clear pitch. Public so it can be tested on its own.
    static float detectPitch (const std::vector<float>& x, double sampleRate);
    static juce::String noteName (int midi, bool withOctave = true);

private:
    void timerCallback() override;
    void pollParams();
    void runPoly();
    void layoutCells();
    void paintSingle (juce::Graphics&, juce::Rectangle<float> area);
    void paintPoly (juce::Graphics&, juce::Rectangle<float> area);
    juce::Rectangle<float> cellRect (int i) const;
    int  getInt (const char* id) const;
    void setInt (const juce::String& id, int v);
    void applyPreset (int index);
    void markCustomIfChanged();
    void nudgeString (int i, int delta);

    StarterAmpProcessor& proc;
    juce::Slider refSlider, shiftSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> refAttach, shiftAttach;
    juce::ComboBox modeBox, presetBox, countBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttach, presetAttach;
    juce::TextButton nudgeDown[PolyTuner::kMaxStrings], nudgeUp[PolyTuner::kMaxStrings];

    // single note
    float freq = 0.0f, cents = 0.0f, rms = 0.0f;
    int midi = -1, missed = 100;

    // all strings
    PolyTuner poly;
    PolySettleGate settle;                                // b111: readings of a string are held back for 0.6 s after its pluck
    PolyTuner::Result polyRes;
    std::vector<float> polyIn;
    bool  polyShown[PolyTuner::kMaxStrings] {}, polyApprox[PolyTuner::kMaxStrings] {};
    float polyCents[PolyTuner::kMaxStrings] {}, polyHz[PolyTuner::kMaxStrings] {};
    float polySince[PolyTuner::kMaxStrings] {};          // seconds since the string was last heard
    double polyRate = 0.0;
    int tick = 0, polyEvery = 3, shownCount = 0, shownMode = -1;
    bool polyListening = true;
};

//==============================================================================
// Input calibration wizard (b94). Opens over the page like the tuner but does not mute: step 1 you stay quiet (noise floor, hum), step 2 you play
// your loudest normal part; the processor's InputCalibrator measures both and this panel shows what it found and the Input / Gate / De-Noise
// settings it suggests. APPLY writes the ticked ones; nothing changes before that.
//==============================================================================
class CalibView : public Panel, private juce::Timer
{
public:
    explicit CalibView (StarterAmpProcessor& p);
    ~CalibView() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

private:
    void timerCallback() override;
    void refreshButtons();
    void applyAdvice();
    float getFloat (const char* id) const;
    void setFloat (const char* id, float v);
    static juce::String fmtDb (float v) { return juce::String (v, 1) + " dB"; }

    StarterAmpProcessor& proc;
    juce::TextButton startBtn { "START" }, applyBtn { "APPLY" }, closeBtn { "CLOSE" };
    juce::ToggleButton tickIn { "Input level" }, tickGate { "Gate" }, tickHum { "De-Noise (hum)" };
    int shownPhase = -1, shownRun = -1;
    InputCalibrator::Result res;
};

//==============================================================================
// Mic position display for the built-in cab blocks. Left: the speaker seen from the front, with one
// coloured marker per block (centre = cap, edge = cone edge). Right: the speaker from the side, with
// the selected block's mic at its real distance and angle. Drag the markers to set Position, Distance
// and Angle (drag the small dot at the back of the mic to tilt it; the mouse wheel tilts too).
//==============================================================================
class MicView : public Panel, public juce::SettableTooltipClient, private juce::Timer
{
public:
    explicit MicView (juce::AudioProcessorValueTreeState&);
    ~MicView() override { stopTimer(); }

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    struct Geo
    {
        juce::Point<float> fc;                 // centre of the front view
        float fr = 0.0f;                       // its outer radius
        juce::Rectangle<float> side, chips;
        float x0 = 0.0f, yc = 0.0f, k = 1.0f, rr = 1.0f;   // side view: baffle x, axis y, pixels per inch, cone radius
    };
    Geo geo() const;
    float val (const char* base, int blk) const;
    void setParam (const char* base, int blk, float v);
    bool usable (int blk) const;
    juce::Point<float> markerPos (const Geo&, int blk) const;
    juce::Point<float> micHead (const Geo&, int blk) const;
    juce::Rectangle<float> chipRect (const Geo&, int blk) const;
    void beginGestures();
    void endGestures();
    void applyDrag (juce::Point<float>);
    void timerCallback() override;

    juce::AudioProcessorValueTreeState& apvts;
    int selected = 0, dragMode = 0;            // dragMode: 0 none, 1 front view, 2 move the mic, 3 tilt the mic
    bool gesturesOpen = false;
    std::vector<float> lastState;
};

//==============================================================================
// Spectrum display for the EQ page: what the sound looks like right now. Shows the final
// output (and optionally the signal before the Post EQ, so you can see what the EQ is doing),
// on a log frequency axis, with peak hold and a peak / RMS readout.
//==============================================================================
class SpectrumView : public Panel, private juce::Timer
{
public:
    explicit SpectrumView (StarterAmpProcessor&);
    ~SpectrumView() override { stopTimer(); }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int kFftOrder = 12;                // 4096 points
    static constexpr int kFftSize  = 1 << kFftOrder;
    static constexpr int kPoints   = 240;               // points across the screen, 20 Hz to 20 kHz

    void timerCallback() override;
    void analyse (const StarterAmpProcessor::AnalyserTap& tap, std::array<float, kPoints>& shown,
                  std::array<float, kPoints>& peaks, float& peakDb, float& rmsDb);

    StarterAmpProcessor& proc;
    juce::dsp::FFT fft { kFftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) kFftSize, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> fftData, samples;

    juce::TextButton modeButtons[3];                    // output / pre-EQ / both
    juce::TextButton tiltButton { "TILT" }, holdButton { "HOLD" };
    int mode = 0;

    std::array<float, kPoints> outShown {}, outPeaks {}, preShown {}, prePeaks {};
    float outPeakDb = -120.0f, outRmsDb = -120.0f, prePeakDb = -120.0f, preRmsDb = -120.0f;
};

//==============================================================================
class EqCurveView : public juce::Component, private juce::Timer
{
public:
    explicit EqCurveView (StarterAmpProcessor& p) : proc (p) { startTimerHz (15); }
    ~EqCurveView() override { stopTimer(); }

    void paint (juce::Graphics&) override;

private:
    static constexpr int kPoints = 120;
    void timerCallback() override;

    StarterAmpProcessor& proc;
    std::array<float, kPoints> curve {};
    std::array<float, kEqBands + 6> lastValues {};
    bool enabled = false;
};

//==============================================================================
// The Post EQ section: on/mode row, the curve, nine faders, and Q / Level knobs.
//==============================================================================
class PostEqPanel : public Panel
{
public:
    explicit PostEqPanel (StarterAmpProcessor& p) : Panel ("POST EQ"), curve (p) {}

    void setup (std::vector<std::unique_ptr<FaderControl>>& fadersIn, std::vector<KnobControl*> knobsIn);
    void resized() override;

private:
    EqCurveView curve;
    std::vector<FaderControl*> faders;
    std::vector<KnobControl*> knobs;
};

// The window's background and header, plus the five section banners
// (PRE FX, AMP, CAB, EQ, POST FX) drawn above their rows of panels.
class Backdrop : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void refresh() { cache = juce::Image(); repaint(); }     // the accent changed

    struct Section { juce::String name, contents; int y; };
    std::vector<Section> sections;

private:
    void paintBackground (juce::Graphics&);
    juce::Image cache;
};

//==============================================================================
// v61: a large illustration of the selected amp (a head on a cab) that fills the right side of the AMP page. It wears the
// same colours and cloth as the amp's panel (see ampThemeFor). Decoration only.
//==============================================================================
class AmpHero : public juce::Component
{
public:
    AmpHero() { setInterceptsMouseClicks (false, false); }
    void setLook (int newTheme, const juce::String& newName)
    {
        if (newTheme == theme && newName == name)
            return;
        theme = newTheme;  name = newName;
        cache = juce::Image();
        repaint();
    }
    // the live positions (0 to 1) of the six knobs drawn on the faceplate: gain, bass, low mid, high mid, treble, presence
    void setValues (const std::array<float, 6>& v)
    {
        if (v != vals) { vals = v; repaint(); }
    }
    void setVoicing (int v) { if (v != voicing) { voicing = v; repaint(); } }
    void paint (juce::Graphics&) override;
    void refresh() { cache = juce::Image(); repaint(); }
    // which faceplate layout a look uses: 0 classic plate, 1 vintage top strip over grille cloth, 2 modern lit panel with a control strip
    static int styleFor (int theme);
private:
    struct Geo { juce::Rectangle<float> head, plate, strip; juce::Point<float> knob[6]; float rad = 10.0f, labelY = 0.0f; int style = 0; };
    Geo geo (float w, float h) const;
    void render (juce::Graphics&, int w, int h) const;
    int theme = 2, voicing = 0;
    juce::String name;
    juce::Image cache;
    std::array<float, 6> vals { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.4f };
};

//==============================================================================
// What a dropdown item does (empty if there is no description). Used by the menus' hover help.
juce::String comboItemTip (const juce::String& paramId, const juce::String& itemText);

// A drop-down menu whose items have hover help: hovering the closed menu describes the current item,
// and hovering an item in the open menu describes that item.
class TipCombo : public juce::ComboBox
{
public:
    juce::String paramId;
    void showPopup() override;
    juce::String getTooltip() override;
};

//==============================================================================
// The MIDI window: which controller (CC 0 to 127, or pitch bend) moves which parameter, the MIDI
// channel, and Learn. Program Change n loads factory preset n. Right-clicking any knob also offers Learn.
//==============================================================================
class MidiPanel : public juce::Component, private juce::ListBoxModel, private juce::Timer
{
public:
    explicit MidiPanel (StarterAmpProcessor&);
    ~MidiPanel() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override { if (isShowing()) startTimerHz (8); else stopTimer(); }

private:
    int  getNumRows() override { return (int) rows.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void timerCallback() override;
    void refreshRows();
    int  selectedSlot() const { const int r = list.getSelectedRow(); return juce::isPositiveAndBelow (r, (int) rows.size()) ? rows[(size_t) r] : -1; }

    StarterAmpProcessor& processor;
    std::vector<int> rows;                 // the slots that have a mapping
    std::vector<int> sortedParams;         // parameter indices in the order the menu shows them
    juce::ListBox list { {}, this };
    juce::ComboBox channelBox, paramBox, slotBox;
    juce::TextButton learnButton { "LEARN" }, bindButton { "ASSIGN" }, removeButton { "REMOVE" },
                     invertButton { "INVERT" }, defaultsButton { "DEFAULTS" };
    juce::Label channelLabel, status, programNote;
    int lastActivity = -1;
};

class MidiWindow : public juce::DocumentWindow
{
public:
    explicit MidiWindow (StarterAmpProcessor& p, juce::LookAndFeel& laf);
    ~MidiWindow() override;
    void closeButtonPressed() override { setVisible (false); }
private:
    MidiPanel panel;
};

//==============================================================================
// A small window that holds one of the tool panels (effect order, metronome, preset browser).
//==============================================================================
class ToolWindow : public juce::DocumentWindow
{
public:
    ToolWindow (const juce::String& title, std::unique_ptr<juce::Component> content, juce::LookAndFeel& laf);
    ~ToolWindow() override;
    void closeButtonPressed() override { setVisible (false); }
};

//==============================================================================
// A switched-off pedal in TIDY VIEW (v73): a small tile with the pedal's name and a switch to bring it back.
//==============================================================================
class PedalTile : public juce::Component
{
public:
    PedalTile (juce::AudioProcessorValueTreeState&, const juce::String& name, const juce::String& description, juce::Colour tint,
               const juce::String& onId, const juce::String& tip);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::String name, description;
    juce::Colour tint;
    juce::ToggleButton sw { "SWITCH ON" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
};

//==============================================================================
// The HELP window (v73): plain-language getting started notes.
//==============================================================================
class HelpPanel : public juce::Component
{
public:
    HelpPanel();
    void resized() override { text.setBounds (getLocalBounds().reduced (10)); }

private:
    juce::TextEditor text;
};

//==============================================================================
// The pedalboard (b46): the signal path is a graph. Pedals, the amps and the cab are boxes with jacks; cables join an output jack to an
// input jack. Drag from a jack to another jack to run a cable, drop a pedal on a cable to put it in the path, click a cable to set its level,
// polarity, pan and channel. Pedals that are switched off wait in the tray.
//==============================================================================
class PatchPanel : public juce::Component, private juce::Timer, private juce::ScrollBar::Listener
{
public:
    explicit PatchPanel (StarterAmpProcessor&);
    ~PatchPanel() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override { if (isShowing()) rebuild(); }      // b51: the timer runs from construction (see the constructor): this panel lives in a window that is shown after the panel itself, and JUCE does not tell a child when its window appears
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hoverCable = -1; hoverNode = -1; repaint(); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed (const juce::KeyPress&) override;
    std::function<void (int)> onOpenNode;        // double-click a box: its controls (node numbers: 0..27 pedals, 28 NAM pedal 1, 29 Amp A, 30 Amp B, 31 Cab)

private:
    using Data = PatchGraph::Data;
    static constexpr int kV = PatchGraph::kNodes + 1;              // the nodes, and the back half of Amp A (its return jack and output) as the last one
    struct VCable { int from = 0, fp = 0, to = 0, tp = 0, chan = 0, first = 0, last = 0; float lvl = 0.0f, pan = 0.0f; bool pol = false, ok = true; };
    struct Jack { int node = 0, port = 0, v = 0; bool out = false; juce::Point<float> p; };
    struct Key { int from = -1, fp = 0, to = -1, tp = 0; bool valid() const { return from >= 0; } };

    // model and layout (all in "content" coordinates; the view scrolls over them)
    void rebuild();
    bool nodeOn (int node) const;
    void toggleNode (int node);
    bool nodeHidden (int node) const;
    juce::Point<float> jackPos (int node, int port, bool out) const;
    void cablePath (juce::Point<float> a, juce::Point<float> b, juce::Path& path) const;
    int  cableAt (juce::Point<float> c, float maxDist, int skipNode = -1) const;
    bool jackAt (juce::Point<float> c, float maxDist, bool wantOut, Jack& found) const;
    int  boxAt (juce::Point<float> c) const;                      // the vertex under a point, or -1
    bool loopLinkAt (juce::Point<float> c) const;
    juce::Rectangle<float> trayChip (int i) const;
    int  trayChipAt (juce::Point<float> p) const;
    void placeNode (Data& d, int node) const;
    bool canWire (int fromNode, int fp, int toNode, int tp) const;
    juce::Point<float> toContent (juce::Point<float> p) const { return p - canvasArea.getPosition().toFloat() + juce::Point<float> (sx, sy); }
    void select (const Key& k, int node);
    void refreshInspector();
    void applyToSelectedCable (const std::function<void (PatchGraph::Cable&)>&);
    void timerCallback() override;
    void scrollBarMoved (juce::ScrollBar*, double) override { sx = (float) hbar.getCurrentRangeStart(); sy = (float) vbar.getCurrentRangeStart(); repaint(); }
    void updateScrollBars();
    void drawBox (juce::Graphics&, int v) const;
    void drawCable (juce::Graphics&, juce::Point<float> a, juce::Point<float> b, juce::Colour, float width, bool selected, bool hovered, bool dashed) const;
    juce::String cableBadge (const VCable&) const;

    StarterAmpProcessor& processor;
    Data data;
    std::unique_ptr<PatchGraph::Plan> plan;
    std::string lastText;
    std::vector<char> lastOn;
    std::vector<VCable> vc;
    std::vector<Jack> jacks;
    std::vector<int> tray;                                         // nodes that wait in the tray (pedals that are off, Amp B off, unused splits)
    bool shown[kV] = {};
    int layerOf[kV] = {};
    float rowOf[kV] = {};
    juce::Rectangle<float> box[kV];
    float contentW = 0.0f, contentH = 0.0f, sx = 0.0f, sy = 0.0f;
    juce::Rectangle<int> canvasArea, trayArea, inspectorArea;
    juce::ScrollBar hbar { false }, vbar { true };

    // selection and dragging
    Key selKey;  int selNode = -1;
    enum class Drag { none, wire, rewire, node } drag = Drag::none;
    int dragNode = -1, wireNode = -1, wireFp = 0, hoverCable = -1, hoverNode = -1;
    bool dragFromTray = false, dragMoved = false, hoverTray = false, hoverLoop = false;
    PatchGraph::Cable held;                                        // the cable being moved by its end (rewire)
    juce::Point<float> mouseAt, downAt;
    bool showAll = false;

    // inspector
    juce::Label nameLabel, noteLabel, levelLabel, panLabel, chanLabel, helpLabel;
    juce::Slider levelSlider, panSlider, xoverSlider;
    juce::ComboBox chanBox;
    juce::ToggleButton polButton { "Flip polarity" }, showAllButton { "Show every pedal" };
    juce::TextButton removeButton { "Remove" }, openButton { "Open controls" }, resetButton { "RESET BOARD" }, dryButton { "Add dry path" };
};

//==============================================================================
// Metronome settings
//==============================================================================
class MetroPanel : public juce::Component
{
public:
    explicit MetroPanel (juce::AudioProcessorValueTreeState&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using APVTS = juce::AudioProcessorValueTreeState;
    juce::ToggleButton onButton { "Metronome on" }, accentButton { "Accent the first beat" };
    juce::ComboBox subBox, soundBox;
    juce::Slider beatsSlider, volumeSlider;
    juce::Label beatsLabel, subLabel, soundLabel, volumeLabel, tempoNote;
    std::unique_ptr<APVTS::ButtonAttachment> onAttach, accentAttach;
    std::unique_ptr<APVTS::ComboBoxAttachment> subAttach, soundAttach;
    std::unique_ptr<APVTS::SliderAttachment> beatsAttach, volumeAttach;
};

//==============================================================================
// Looper window (b16): one big button that steps Record -> Play -> Overdub -> Play, plus Undo, Stop and Clear.
//==============================================================================
class LooperPanel : public juce::Component, private juce::Timer
{
public:
    explicit LooperPanel (StarterAmpProcessor&);
    ~LooperPanel() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override;
    static juce::String stateText (int state);
    static juce::String buttonText (int state);
    static juce::Colour stateColour (int state);

private:
    using APVTS = juce::AudioProcessorValueTreeState;
    void timerCallback() override;
    StarterAmpProcessor& processor;
    juce::TextButton cycleButton, undoButton { "UNDO" }, stopButton { "STOP" }, clearButton { "CLEAR" };
    juce::ToggleButton armButton { "Wait for the first note before recording" }, latchButton { "My footswitch is latching (sends on / off alternately)" };
    juce::ComboBox barsBox, modeBox;
    juce::Slider levelSlider;
    juce::Label barsLabel, levelLabel, modeLabel, note;
    std::unique_ptr<APVTS::ButtonAttachment> armAttach, latchAttach;
    std::unique_ptr<APVTS::ComboBoxAttachment> barsAttach, modeAttach;
    std::unique_ptr<APVTS::SliderAttachment> levelAttach;
    int shownState = -1;
};

//==============================================================================
// Modulators window (b107): two modulators (your playing level, an LFO or a random source), each moving up to three knobs.
//==============================================================================
class ModSourcePanel : public juce::Component, private juce::Timer
{
public:
    explicit ModSourcePanel (StarterAmpProcessor&);
    ~ModSourcePanel() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using APVTS = juce::AudioProcessorValueTreeState;
    void timerCallback() override;
    StarterAmpProcessor& processor;
    struct Column
    {
        juce::ComboBox source, target[3], div, len;                                  // div, len, steps: the Step sequence (b120)
        juce::Slider rate, smooth, amount[3], steps[16];
        juce::Label rateLabel, smoothLabel, title;
        std::unique_ptr<APVTS::ComboBoxAttachment> sourceAtt, targetAtt[3], divAtt, lenAtt;
        std::unique_ptr<APVTS::SliderAttachment> rateAtt, smoothAtt, amountAtt[3], stepAtt[16];
    } col[2];
    juce::Label note;
    int shownType[2] { -1, -1 }, shownStep[2] { -1, -1 };
    float shownValue[2] { 0.0f, 0.0f };
};

//==============================================================================
// Tone matching: load a reference recording, play your guitar through the amp, and let the Post EQ move
// your tone toward the reference. It matches the overall tonal balance only.
//==============================================================================
class ToneMatchPanel : public juce::Component, private juce::Timer
{
public:
    explicit ToneMatchPanel (StarterAmpProcessor&);
    ~ToneMatchPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

    // Used by the interface and by the tests.
    bool loadReferenceFile (const juce::File&);
    void applyMatch();
    void resetEq();
    void clearYours();
    void feed (const float* frame8192);              // adds one frame of your playing (the timer does this while listening)
    int  yourFrames() const { return yours.frames; }
    bool hasReference() const { return haveRef; }
    std::array<float, kEqBands> lastFit {};

private:
    void timerCallback() override;
    void setListening (bool);
    void chooseReference();
    void showReferenceFromState();
    void updateStatus();
    juce::Rectangle<float> graphArea() const;

    StarterAmpProcessor& processor;
    ToneMatch::Analyzer yours;
    std::array<float, ToneMatch::kBands> target {};
    bool haveRef = false;
    juce::String refName;
    int samplesSince = 0, lastPos = 0;
    juce::TextButton loadButton { "LOAD REFERENCE..." }, listenButton { "LISTEN" }, clearButton { "CLEAR MINE" },
                     applyButton { "APPLY TO EQ" }, resetButton { "RESET EQ" };
    juce::Slider strength;
    juce::Label strengthLabel, status, help;
    std::unique_ptr<juce::FileChooser> chooser;
};

//==============================================================================
// Tone Lab (v75): load a clean DI recording and a recording of the sound you want, and let the plugin search its own
// settings for the closest match (see ToneLab.h). It runs on a private copy of the plugin in a background thread.
//==============================================================================
class ToneLabPanel : public juce::Component, private juce::Timer
{
public:
    explicit ToneLabPanel (StarterAmpProcessor&);
    ~ToneLabPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Worker;
    void timerCallback() override;
    void chooseFile (bool di);
    bool loadFile (bool di, const juce::File&);
    void start();
    void finish();
    void applyResult();
    void undoApply();
    void updateButtons();
    juce::Rectangle<float> graphArea() const;

    StarterAmpProcessor& processor;
    std::vector<float> diMono;
    double diSr = 48000.0;
    ToneLab::Features targetFeat;
    bool haveDi = false, haveTarget = false;
    juce::String diName, targetName;

    std::unique_ptr<StarterAmpProcessor> probe;
    std::unique_ptr<ToneLab::Fitter> fitter;
    std::unique_ptr<Worker> worker;
    std::atomic<bool> cancel { false }, running { false }, finished { false };
    juce::CriticalSection lock;
    float progressValue = 0.0f;
    juce::String progressText;
    double progressBarValue = 0.0;
    ToneLab::Result result;
    bool haveResult = false;
    std::vector<std::pair<juce::String, float>> previous;
    bool applied = false;

    juce::TextButton diButton { "LOAD DI..." }, targetButton { "LOAD TARGET..." }, startButton { "START MATCH" },
                     cancelButton { "CANCEL" }, applyButton { "APPLY TO MY SOUND" }, undoButton { "UNDO" };
    juce::ToggleButton optAmp { "Try other amps" }, optCab { "Try cab speakers and mics" },
                       optPedals { "Try compressor, overdrive and boost" }, optEq { "Finish with the Post EQ" };
    juce::Label diLabel, targetLabel, status, summary, help;
    juce::ProgressBar bar { progressBarValue };
    std::unique_ptr<juce::FileChooser> chooser;
};

//==============================================================================
// The preset browser: Factory and User tabs, a search box and a tag filter. User presets can be
// renamed, re-tagged and deleted here. Damaged files in the folder are listed but cannot be loaded.
//==============================================================================
class PresetBrowser : public juce::Component, private juce::ListBoxModel
{
public:
    struct Callbacks
    {
        std::function<void (int)> loadFactory;
        std::function<void (const juce::File&)> loadFile;
        std::function<void()> saveCurrent;
        std::function<void()> changed;
        std::function<juce::File()> folder;
    };
    PresetBrowser (StarterAmpProcessor&, Callbacks);
    void refresh();                       // scans the user folder again
    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override { if (isShowing()) refresh(); }

    static juce::String cleanTags (const juce::String& text);
    static juce::String readTags (const juce::File& file, bool& damaged);
    static bool writeTags (const juce::File& file, const juce::String& tags);

private:
    struct Entry { juce::String name, tags; int factoryIndex = -1; juce::File file; bool damaged = false; };
    int  getNumRows() override { return (int) view.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override { loadRow (row); }
    void returnKeyPressed (int row) override { loadRow (row); }
    void selectedRowsChanged (int) override { updateButtons(); }
    void rebuildView();
    void rebuildTagMenu();
    void updateButtons();
    void loadRow (int row);
    const Entry* selectedEntry() const;
    bool onUserTab() const { return userTab.getToggleState(); }
    void say (const juce::String& text) { status.setText (text, juce::dontSendNotification); }

    StarterAmpProcessor& processor;
    Callbacks cb;
    std::vector<Entry> factoryEntries, userEntries;
    std::vector<int> view;                // indices into the list shown (factory or user)
    juce::TextButton factoryTab { "FACTORY" }, userTab { "USER" };
    juce::TextEditor search;
    juce::ComboBox tagBox;
    juce::ListBox list { {}, this };
    juce::TextButton loadButton { "LOAD" }, saveButton { "SAVE CURRENT..." }, renameButton { "RENAME" },
                     tagsButton { "EDIT TAGS" }, deleteButton { "DELETE" }, folderButton { "SHOW FOLDER" };
    juce::Label status;
    juce::StringArray tagChoices;
};

//==============================================================================
// Riff recorder (v52): record a phrase and loop it back. DI mode records the guitar at the input, so the whole
// chain processes the playback; Full tone mode records the finished sound at the output.
//==============================================================================
class RiffPanel : public juce::Component, private juce::Timer
{
public:
    explicit RiffPanel (StarterAmpProcessor&);
    ~RiffPanel() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using APVTS = juce::AudioProcessorValueTreeState;
    void timerCallback() override;
    StarterAmpProcessor& processor;
    juce::TextButton recButton { "REC" }, playButton { "PLAY" }, stopButton { "STOP" }, clearButton { "CLEAR" },
                     exportButton { "EXPORT WAV..." }, importButton { "IMPORT WAV..." };
    juce::ComboBox modeBox;
    juce::ToggleButton loopButton { "Loop" }, muteButton { "Mute my live playing while the riff plays" };
    juce::Slider levelSlider;
    juce::Label modeLabel, levelLabel, status, note;
    std::unique_ptr<APVTS::ComboBoxAttachment> modeAttach;
    std::unique_ptr<APVTS::ButtonAttachment> loopAttach, muteAttach;
    std::unique_ptr<APVTS::SliderAttachment> levelAttach;
    juce::Rectangle<int> waveArea;
    std::vector<float> peaks;
    int lastLen = -1, lastState = -1;
    std::unique_ptr<juce::FileChooser> chooser;
    void updateButtons();
};

class StarterAmpEditor : public juce::AudioProcessorEditor,
                         private juce::Timer
{
public:
    explicit StarterAmpEditor (StarterAmpProcessor&);
    ~StarterAmpEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    using APVTS = juce::AudioProcessorValueTreeState;

    struct MidiWatcher : private juce::Timer
    {
        std::function<void()> onTick;
        void begin() { startTimerHz (8); }
        ~MidiWatcher() override { stopTimer(); }
        void timerCallback() override { if (onTick) onTick(); }
    };
    MidiWatcher midiWatcher;
    juce::TextButton midiButton { "MIDI" };
    std::unique_ptr<MidiWindow> midiWindow;
    int lastPresetLoadCounter = 0, lastMidiActivity = 0;
    void openMidiWindow();
    void midiTick();
    void openOrderWindow();
    void openBlockControls (int block);
    void openMetroWindow();
    void openBrowserWindow();
    void openToneWindow();
    void openLoopWindow();
    void openModsWindow();
    void openToneLabWindow();
    void abMenuClicked();
    bool userBusy() const;
    juce::TextButton riffButton { "RIFF" };
    std::unique_ptr<ToolWindow> riffWindow;
    void openRiffWindow();
    juce::TextButton browseButton { "BROWSE" }, abA { "A" }, abB { "B" }, abMenu { "A/B" },
                     undoButton { "UNDO" }, redoButton { "REDO" }, metroButton { "METRO" }, orderButton { "ORDER" }, loopButton { "LOOP" }, modsButton { "MODS" };
    std::unique_ptr<ToolWindow> loopWindow, modsWindow, orderWindow, metroWindow, browserWindow, toneWindow, toneLabWindow, libraryWindow, helpWindow;
    juce::TextButton toneMatchButton { "MATCH..." }, toneLabButton { "TONE LAB..." };
    mutable juce::uint32 lastTextBusy = 0;     // when a text box last had the keyboard (typing a value is a hand edit)

    struct ToggleControl
    {
        juce::ToggleButton button;
        std::unique_ptr<APVTS::ButtonAttachment> attachment;
    };
    struct ComboControl
    {
        TipCombo box;
        std::unique_ptr<APVTS::ComboBoxAttachment> attachment;
        std::unique_ptr<juce::ParameterAttachment> paramAttach;   // used by the amp menu, which lists Acoustic first
    };
    juce::ComboBox& makeAmpModelCombo (const char* paramId);
    juce::ComboBox& makeGroupedCombo (const char* paramId);

    // ---- per-block presets (see BlockPresets.cpp section at the end of PluginEditor.cpp)
    void setupBlockPresets();
    void showBlockPresetMenu (juce::Component& anchor, const juce::String& family, int slot, std::function<std::vector<juce::String> (int)> idsFor);
    juce::ComboBox& makeSpeakerCombo (const char* paramId);
    juce::ComboBox& makeMicCombo (const char* paramId);   // v71
    juce::ComboBox& makeCabinetCombo (const char* paramId);
    void syncVisibility();
    KnobControl *fuzzStarveKnob = nullptr, *fuzzInputKnob = nullptr, *fuzzCompKnob = nullptr, *fuzzMidsKnob = nullptr, *fuzzDriftKnob = nullptr, *fuzzHissKnob = nullptr, *fuzzShapeKnob = nullptr, *fuzzCrushKnob = nullptr, *fuzzBitsKnob = nullptr, *fuzzRateKnob = nullptr;
    juce::Component *fuzzGeToggle = nullptr, *fuzzOpAmpToggle = nullptr, *fuzzVersionBox = nullptr, *fuzzModelBox = nullptr,
                    *fuzzDiodeBox = nullptr, *fuzzTonePassToggle = nullptr, *fuzzBufferToggle = nullptr;
    KnobControl *focusKnob = nullptr, *masterKnob = nullptr, *fuzzGateKnob = nullptr, *fuzzStabKnob = nullptr, *buzzLowKnob = nullptr, *buzzHighKnob = nullptr, *limCeilKnob = nullptr, *limRelKnob = nullptr, *shimmerDetuneKnob = nullptr, *shimmerBelowKnob = nullptr, *delayOffsetKnob = nullptr, *delaySpreadKnob = nullptr, *preOffsetKnob = nullptr, *preSpreadKnob = nullptr,
                *shimmerAmountKnob = nullptr, *shimmerToneKnob = nullptr;
    juce::Component* shimmerPitchBox = nullptr;
    KnobControl *shimmerAmountKnob2 = nullptr, *shimmerToneKnob2 = nullptr, *shimmerDetuneKnob2 = nullptr, *shimmerBelowKnob2 = nullptr, *reverbPreKnob2 = nullptr;
    juce::Component *shimmerPitchBox2 = nullptr, *shimmerModeBox2 = nullptr;
    juce::Component* shimmerModeBox = nullptr;
    juce::Component* ampQualityBox = nullptr;
    std::vector<KnobControl*> electricAmpKnobs;
    KnobControl* presenceKnobA = nullptr;   // b38: relabelled Cut on the two amps that have a Cut control instead of Presence
    KnobControl* presenceKnobB = nullptr;
    KnobControl* jadeDistKnob = nullptr;        // b39: Jade Chorus Distortion (shown only for that amp)
    std::vector<KnobControl*> blendKnobs;       // v0.2 Triple Blend: the three part levels (shown only for that amp)

    // Helpers that create a control, attach it to a parameter, and keep it alive.
    void addKnob (Panel&, const char* paramId, const juce::String& name,
                  const juce::String& suffix = {}, int decimals = 2, int page = -1);
    void addAdvKnob (Panel&, const char* paramId, const juce::String& name,
                     const juce::String& suffix = {}, int decimals = 2, int page = -1);   // shown only while the panel's ADV is on
    void setupAdvanced (Panel&, const char* key);       // remembers the panel's ADV state with the session
    std::vector<std::pair<Panel*, juce::String>> advancedPanels;
    std::vector<juce::Component*> ampRealismComps;      // Amp A realism menus (hidden for the acoustic model)
    KnobControl *wowHeelKnob = nullptr, *wowToeKnob = nullptr, *wowGritKnob = nullptr;
    KnobControl* octKnob[10] = {};  KnobControl* octGritKnob = nullptr;  juce::Component *octABox = nullptr, *octBBox = nullptr, *octDryFxBox = nullptr;       // b79
    juce::Component *octTrackBox = nullptr, *octCharBox = nullptr;  KnobControl *octAtkKnob = nullptr, *octDecKnob = nullptr;     // b86: hidden in the analogue-divider mode
    juce::ToggleButton& makeToggle (const char* paramId, const juce::String& text);
    juce::ComboBox& makeCombo (const char* paramId);
    juce::ComboBox& makeIndexCombo (const char* paramId);      // b101: item id = parameter index + 1, for menus whose item count changes

    // Grey-out system (v52): controls that do nothing in the current state are greyed, with the reason in the tooltip.
    // Every knob, switch and menu is registered here by its parameter id.
    std::map<juce::String, juce::Component*> idMap;
    std::map<juce::Component*, juce::String> baseTips;
    std::vector<std::pair<Panel*, juce::String>> masterSwitches;   // a panel and the id of its ON switch
    void setUsed (const char* id, bool used, const juce::String& why = {});
    float pv (const char* id) const;
    void syncUsed();

    // A footswitch that is momentary or latching, depending on its Latch parameter.
    void setupSwitch (juce::TextButton&, const char* paramId, const char* latchId,
                      std::unique_ptr<juce::ParameterAttachment>& attachment);
    bool isLatched (const char* latchId) const;
    void setSwitch (const char* paramId, bool on);
    void chooseIR (int slot);
    void chooseReverbIr();                                   // v0.2: the IR reverb's file
    void updateIRButtons();
    void timerCallback() override { updateDiMatch(); updateIRButtons(); syncOverdrivePage(); syncIRPages(); syncDelayKnobs(); syncVisibility(); syncUsed(); layoutFlow (false); }
    void syncOverdrivePage();
    void syncIRPages();
    void syncDelayKnobs();
    void syncVoicingLabel();
    juce::ComboBox* voicingToggle = nullptr;      // v60: the Voicing switch is a three-position menu (the name is from when it was a button)
    juce::ComboBox* voicingBoxB = nullptr;
    void tapTempo();
    void showPage (int index);

    // v73: pages are laid out by a small flow engine. Every pedal is an 'item'; in TIDY VIEW a pedal that is switched off
    // is replaced by a small tile (a PedalTile) and the pedals that are on move together.
    struct FlowItem
    {
        Panel* main = nullptr;
        Panel* extra = nullptr;                 // a second panel that goes with the first (shown and hidden with it)
        const char* extraId = nullptr;          // b54: if set, the second panel is only there while this switch parameter is on
        bool tallWithAdv = false;               // b113: with ADV on the box uses the full page height (when the lane under it is free), so its knobs are not squeezed
        bool extraWithAdv = false;              // b63: the second panel is only there while the main panel's ADV button is on
        int w = 0, wExtra = 0, lanes = 1, lane = -1;
        juce::String onId, name;
        bool legacy = false;                    // b29: an old stand-alone effect that a Mod FX slot type replaces; hidden while it is off
        std::unique_ptr<PedalTile> tile;
    };
    // b60: the pedals sit in 'inner', a strip as wide as they need. When switching a lot on makes them wider than the page, the
    // strip scrolls sideways with 'bar' (before this they ran off the right edge and could not be reached).
    struct FlowPage : public juce::ScrollBar::Listener
    {
        juce::Component* page = nullptr; std::vector<FlowItem> items; juce::Label hint;
        juce::Component inner;
        juce::ScrollBar bar { false };
        int contentW = 0;
        void scrollBarMoved (juce::ScrollBar*, double start) override { inner.setTopLeftPosition (-(int) std::lround (start), 0); }
    };
    std::vector<std::unique_ptr<FlowPage>> flowPages;
    juce::String flowSignature;
    bool tidyOn = true;
    juce::TextButton tidyButton { "TIDY VIEW" }, helpButton { "HELP" };
    void addFlowPage (juce::Component& page, std::vector<FlowItem> items);
    void layoutFlow (bool force);
    void openHelpWindow();
    // Cab page: two sub-tabs (the cab blocks, and the speaker shaping / export)
    juce::TextButton cabSubTabs[2][2];
    int cabTab = 0;
    void showCabTab (int index);

    StarterAmpProcessor& processorRef;
    AmpLookAndFeel lookAndFeel;
    juce::ComponentBoundsConstrainer constrainer;

    std::vector<std::unique_ptr<KnobControl>>   knobs;
    std::vector<std::unique_ptr<ToggleControl>> toggles;
    std::vector<std::unique_ptr<ComboControl>>  combos;

    juce::TextButton noiseOctButton   { "OCT UP" };
    juce::TextButton noiseOct2Button  { "2 OCT UP" };
    juce::TextButton noiseGrossButton { "CLASH" };
    juce::TextButton noiseRingButton  { "RING" };
    std::unique_ptr<juce::ParameterAttachment> noiseOctAttachment, noiseOct2Attachment,
                                               noiseGrossAttachment, noiseRingAttachment;

    // Page tabs of the Noise panel (they choose which settings are shown, nothing more)
    juce::TextButton ampTabA { "AMP A" }, ampTabB { "AMP B" }, ampTabN { "NAM" }, ampTabM { "MIX" };   // the tabs on the AMP page
    int ampTab = 0;
    void showAmpTab (int index);
    juce::TextButton tabOctave { "OCTAVE" }, tabGross { "CLASH" }, tabRing { "RING MOD" };
    juce::TextButton irButtons[kCabBlocks];
    juce::TextButton tapButton { "TAP" };
    juce::Slider tempoSlider;
    std::unique_ptr<APVTS::SliderAttachment> tempoAttachment;
    std::vector<double> tapTimes;
    KnobControl* preTimeKnob = nullptr;
    KnobControl* postTimeKnob = nullptr;
    juce::TextButton alignButton { "ALIGN NOW" };
    juce::TextButton libraryButton { "IR LIBRARY" };   // v70
    juce::TextButton suggestCabButton { "SUGGEST CAB" };   // b70
    juce::TextButton reverbIrButton { "Load reverb IR..." };   // v0.2
    void openLibraryWindow();
    std::unique_ptr<juce::FileChooser> chooser;

    Backdrop content;   // holds the panels, and is scaled to fit the window
    juce::TooltipWindow tooltipWindow { this, 500 };   // hover help for every control
    Panel inputPanel      { "INPUT & GATE" };
    Panel noisePanel      { "CLASH PEDAL" };
    Panel fuzzPanel       { "FUZZ" };
    Panel fuzz2Panel      { "FUZZ 2" };
    struct FuzzCtl { juce::Component *vProg = nullptr, *vOrder = nullptr, *vHold = nullptr, *buzzSoft = nullptr, *buzzHard = nullptr, *buzzPol = nullptr, *ge = nullptr, *opAmp = nullptr, *version = nullptr, *model = nullptr, *diode = nullptr, *tonePass = nullptr, *buffer = nullptr;
                     KnobControl *squeal = nullptr, *oscHz = nullptr, *action = nullptr, *actSpeed = nullptr, *gate = nullptr, *stab = nullptr, *starve = nullptr, *mids = nullptr, *shape = nullptr, *crush = nullptr, *bits = nullptr, *rate = nullptr, *comp = nullptr, *input = nullptr, *drift = nullptr, *hiss = nullptr, *buzzLow = nullptr, *buzzHigh = nullptr, *buzzMid = nullptr, *octUp = nullptr, *buzzBlend = nullptr, *tone = nullptr, *oct = nullptr, *boost = nullptr, *vAmt = nullptr, *vDecay = nullptr, *vMod = nullptr, *cut = nullptr, *eq1 = nullptr, *eq2 = nullptr, *eq3 = nullptr, *eq4 = nullptr, *eq5 = nullptr, *depth = nullptr; } fzc[2];
    Panel preDelayPanel   { "DELAY 1" };
    Panel compPanel       { "COMPRESSOR" };
    GainReductionMeter grMeter { processorRef };
    Panel phaserPanel     { "PHASER" };
    Panel chorusPanel     { "CHORUS" };
    Panel overdrivePanel  { "OVERDRIVE" };
    Panel drive2Panel     { "DRIVE 2" };
    Panel transposePanel  { "TRANSPOSE / FORMANT" };
    Panel diMatchPanel    { "DI MATCH" };                          // b34
    juce::TextButton diLearnButton { "LEARN" }, diLoadButton { "LOAD DI..." }, diClearButton { "CLEAR" };
    juce::Label diStatusLabel;
    void chooseDiReference();
    void updateDiMatch();

    // The five pages and their tabs
    juce::Component pageInput, pageGain, pageMod, pageDelay, pagePitch, pageAmp, pageCab, pageSpeaker, pageEq, pagePost;   // b61: pageDelay is the TIME page
    juce::TextButton pageTabs[9];
    // b56: right-click a page tab to lock the page against preset changes
    struct TabWatcher : juce::MouseListener
    {
        std::function<void()> onPopup;
        void mouseDown (const juce::MouseEvent& e) override { if (e.mods.isPopupMenu() && onPopup) onPopup(); }
    };
    TabWatcher tabWatch[9];
    void buildParamPages();                 // tells the processor which page each parameter sits on
    void showTabMenu (int page);
    void refreshTabLocks();
    Panel harmPanel       { "HARMONIZER" };
    juce::Label harmHeard;                    // v53: the notes the harmonizer hears
    Panel preDelayFxPanel  { "DELAY 1: DUCK, MODULATION, DIFFUSION, HOLD, TAPS" };
    Panel postDelayFxPanel { "DELAY 2: DUCK, MODULATION, DIFFUSION, HOLD, TAPS" };
    AmpHero ampHero;                  // v61: the big amp illustration on the right of the AMP page
    Panel ampArtPanel     { "" };      // the amp's grille cloth under its controls (decoration)
    void applyAccent (juce::Colour);
    juce::Colour currentAccent;
    Panel wowPanel        { "PITCH PEDAL  (EXPRESSION)" };
    Panel octPanel        { "OCTAVER" };
    Panel synthPanel      { "GUITAR SYNTH  (SINGLE NOTE)" };
    juce::Label synthHeard;                   // v0.2: the note the synth is playing
    Panel shapePanel      { "BLOCK SHAPING  (per cab block)" };
    Panel dynPanel        { "SPEAKER DYNAMICS" };
    Panel exportPanel     { "EXPORT CAB AS IR" };
    juce::TextButton shapeTabs[4];
    juce::TextButton exportButton { "EXPORT WAV..." };
    juce::Label exportStatus;
    void exportCab();
    Panel grainPanel      { "GRANULAR CLOUD" };
    Panel tremPanel       { "TREMOLO" };
    Panel wobblePanel     { "WOBBLE" };
    Panel swellPanel      { "SWELL" };
    Panel filterPanel     { "FILTER" };
    Panel volPanPanel     { "VOL / PAN" };
    Panel mbPanel         { "MULTIBAND COMP" };
    Panel mbBandsPanel    { "COMP BANDS" };                           // b55: Threshold, Ratio, Attack, Release and Gain for each of the four bands
    Panel satPanel        { "SATURATOR" };                  // b55
    Panel satBandsPanel   { "SATURATOR BANDS" };                      // b55: Drive and Level for each band
    juce::Label satTypeLbl[4];                                                                  // b63: names for the per-band Type menus
    juce::TextButton satTab[6] { juce::TextButton ("LEVELS"), juce::TextButton ("TYPE"), juce::TextButton ("MIX"), juce::TextButton ("TONE"), juce::TextButton ("DYN"), juce::TextButton ("FDBK") };   // b63: pages of the bands panel (shown with ADV)
    Panel gatePanel       { "GATE / EXPANDER" };
    Panel shaperPanel     { "TRANSIENT SHAPER" };                    // b53 (shares the Gate's place on the board)
    Panel shaperBandsPanel { "SHAPER BANDS" };                        // b54: the four bands' Attack and Sustain; shown beside the shaper while Multiband is on
    KnobControl *tsFocusKnob = nullptr, *tsXoKnob[3] = { nullptr, nullptr, nullptr };
    KnobControl* filtRateKnob = nullptr;
    Panel doublerPanel    { "DOUBLER" };
    Panel widenerPanel    { "WIDENER" };
    KnobControl* tremRateKnob = nullptr;
    // Knobs that go inactive when their note menu is on a note value (tempo sync)
    KnobControl *wahRateKnob = nullptr, *phaserRateKnob = nullptr, *wobRateKnob = nullptr, *swellKnob = nullptr,
                *granSizeKnob = nullptr, *granDensKnob = nullptr, *granPosKnob = nullptr, *reverbPreKnob = nullptr;
    Panel wahPanel        { "WAH" };
    SpectrumView spectrumView { processorRef };

    // Presets (saved as files in Documents/StarterAmp/Presets)
    juce::ComboBox presetBox;
    juce::TextButton presetPrev { "<" }, presetNext { ">" }, presetSave { "SAVE" }, presetDelete { "DELETE" };
    juce::Array<juce::File> presetFiles;
    static juce::File presetFolder();
    void refreshPresets (const juce::String& select = {});
    void loadPresetFile (const juce::File&);
    void loadFactoryPreset (int index);
    void afterPresetLoad (int page);
    void savePresetDialog();
    void deletePreset();
    void stepPreset (int direction);
    Panel ampPanel        { "AMP" };
    Panel metalPanel      { "MODERN METAL" };
    Panel ampBPanel       { "AMP B  (PARALLEL)" };
    Panel metalBPanel     { "AMP B  MODERN METAL" };
    Panel ampMixPanel     { "AMP MIX" };
    Panel lofiPanel       { "LO-FI WARBLE" };
    Panel boostPanel      { "BOOST" };
    Panel boostEqPanel    { "BOOST EQ  (4-BAND PARAMETRIC)" };
    Panel mfPanel0        { "MOD FX A" };
    Panel mfPanel1        { "MOD FX B" };
    Panel mfPanel2        { "MOD FX C" };
    Panel mfPanel3        { "MOD FX D" };
    Panel& mfPanelOf (int s) { return s == 0 ? mfPanel0 : s == 1 ? mfPanel1 : s == 2 ? mfPanel2 : mfPanel3; }
    Panel reverbShapePanel { "REVERB SHAPING" };
    Panel reverb2ShapePanel { "REVERB 2 SHAPING" };
    KnobControl* mfRateKnob[4] = { nullptr, nullptr, nullptr, nullptr };
    KnobControl* mfShiftKnob[4] = { nullptr, nullptr, nullptr, nullptr };
    KnobControl* compThumpKnob[3] = { nullptr, nullptr, nullptr };         // b31
    KnobControl* compLlKnob[3] = { nullptr, nullptr, nullptr };            // b62: Crush, Crunch, Darkness (Crush Limiter style)
    juce::Component* compLlRelBox = nullptr;                                // b62: the Crush Limiter release choice
    KnobControl* compCsKnob = nullptr;                                         // b76: CS-3 Attack
    KnobControl* compOptoKnob[3] = { nullptr, nullptr, nullptr };          // b58: Sustain, Grit, Treble (Opto Sustain style)
    KnobControl* compClassicKnob[3] = { nullptr, nullptr, nullptr };       // b58: Thresh, Attack, Release (hidden in Opto Sustain)
    int lastCompStyleSeen = -1;                                             // b68 (R17): to notice the user switching Comp to the Dyna style
    KnobControl* compMixKnob = nullptr; KnobControl* compOutKnob = nullptr;    // b58: renamed Blend / Volume in Opto Sustain
    juce::Component* compRatioBox = nullptr; juce::Component* compAutoBox = nullptr; juce::Component* compSatBox = nullptr; juce::Component* compDetBox = nullptr;
    KnobControl* mfFbKnob[4] = { nullptr, nullptr, nullptr, nullptr };     // b29: knobs a type does not use are hidden
    KnobControl* mfManKnob[4] = { nullptr, nullptr, nullptr, nullptr };
    KnobControl* mfToneKnob[4] = { nullptr, nullptr, nullptr, nullptr };
    KnobControl* mfFidKnob[4] = { nullptr, nullptr, nullptr, nullptr };
    juce::Component* mfChModeBox[4] = {}; juce::Component* mfFModeBox[4] = {}; juce::Component* mfFMotionBox[4] = {}; juce::Component* mfFlipBox[4] = {};   // b43
    juce::Component* mfHoldBox[4] = {};   // b105
    KnobControl* mfXKnob[4][12] = {};
    KnobControl* mfDepthKnob[4] = {};                                     // b62: the Vocoder has no sweep
    juce::Component* mfFrameBox[4] = {}; juce::Component* mfLoVoiceBox[4] = {}; juce::Component* mfViVoiceBox[4] = {}; juce::Component* mfFlVoiceBox[4] = {}; juce::Component* mfPhVoiceBox[4] = {}; juce::Component* mfPhStereoBox[4] = {}; juce::Component* mfFsModeBox[4] = {}; juce::Component* mfVocBox[4][3] = {};     // b62: Frequency shifter mode; Vocoder carrier, bands, drone note
                                       // b30: Phaser stages / envelope / lag, Lo-Fi noise / dropouts / level

    // The NAM tab: one strip per model slot, each with its own loader (0 Amp A, 1 Amp B, 2 pedal before the
    // overdrive, 3 pedal after the cab)
    struct NamStrip
    {
        Panel panel { "" };
        juce::TextButton load { "LOAD MODEL..." }, clear { "CLEAR" };
        juce::Label info;
        juce::String lastText;
    };
    NamStrip namStrip[4];
    std::unique_ptr<juce::FileChooser> namChooser;
    void chooseNamFile (int slot);
    void loadNamModel (int slot, const juce::File&);
    void updateNamInfo (int slot);
    Panel acousticPanel   { "ACOUSTIC / PIEZO  (used when Amp Model = Acoustic)" };
    Panel irAPanel        { "CAB BLOCK A" };
    Panel irBPanel        { "CAB BLOCK B" };
    Panel irCPanel        { "CAB BLOCK C" };
    Panel irDPanel        { "CAB BLOCK D" };
    Panel* irPanels[kCabBlocks] {};
    Panel cabPanel        { "CAB" };
    std::vector<std::unique_ptr<FaderControl>> eqFaders;
    PostEqPanel eqPanel   { processorRef };
    Panel progEqPanel     { "PROGRAM EQ" };      // b59
    IRView irView         { processorRef };
    MicView micView       { processorRef.apvts };
    MeterView meterView   { processorRef };
    TunerView tunerView   { processorRef };
    CalibView calibView   { processorRef };
    juce::TextButton calibButton { "CALIBRATE INPUT" };
    juce::TextButton tunerButton { "TUNER" };
    juce::Label perfLabel;   // b53: CPU and latency readout in the header
    Panel postDelayPanel  { "DELAY 2" };
    Panel reverbPanel     { "REVERB" };
    Panel reverb2Panel    { "REVERB 2" };
    Panel outputPanel     { "OUTPUT" };
    Panel repeaterPanel   { "REPEATER" };       // b120
    Panel tapeStopPanel   { "TAPE STOP" };      // b116: moved from the OUTPUT panel to the TIME page

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StarterAmpEditor)
};
