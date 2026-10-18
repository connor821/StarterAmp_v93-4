#pragma once
// v70: the IR library window. Included by PluginEditor.cpp only (header-only so the build file does not change).
#include "PluginEditor.h"
#include "IrLibrary.h"

class IrLibraryPanel : public juce::Component, private juce::ListBoxModel, private juce::Timer
{
public:
    explicit IrLibraryPanel (StarterAmpProcessor& p, std::function<void()> sentCallback = {})
        : processor (p), onSent (std::move (sentCallback)),
          library (juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("StarterAmp"))
    {
        library.setReferenceCurves (&StarterAmpProcessor::builtinSpeakerCurves());
        speakerNames = StarterAmpProcessor::builtinSpeakerNames();

        for (auto* b : { &addButton, &rescanButton, &removeButton, &prevButton, &nextButton,
                         &favButton, &hideButton, &sendA, &sendB, &sendC, &sendD })
            addAndMakeVisible (*b);
        for (auto* t : { &favOnly, &hideDupes, &showHidden, &auditionButton })
            addAndMakeVisible (*t);
        auditionButton.setClickingTogglesState (true);
        favOnly.setClickingTogglesState (true);
        hideDupes.setClickingTogglesState (true);
        showHidden.setClickingTogglesState (true);
        hideDupes.setToggleState (true, juce::dontSendNotification);
        for (auto* t : { &favOnly, &hideDupes, &showHidden })
            t->onClick = [this] { rebuildView(); };

        addAndMakeVisible (search);
        search.setTextToShowWhenEmpty ("Search names and folders", ui::textDim);
        search.onTextChange = [this] { rebuildView(); };

        addAndMakeVisible (tree);
        tree.setRootItemVisible (false);
        tree.setDefaultOpenness (true);
        tree.setColour (juce::TreeView::backgroundColourId, ui::panel);
        tree.setColour (juce::TreeView::linesColourId, ui::panelEdge);

        addAndMakeVisible (list);
        list.setRowHeight (26);
        list.setColour (juce::ListBox::backgroundColourId, ui::panel);
        list.setColour (juce::ListBox::outlineColourId, ui::panelEdge);

        addAndMakeVisible (compareBox);
        compareBox.addItem ("Compare with: closest built-in speaker", 1);
        for (int b = 0; b < kCabBlocks; ++b)
            compareBox.addItem ("Compare with: cab block " + cabLetter (b) + " (current)", 2 + b);
        compareBox.addItem ("Compare with: nothing", 6);
        compareBox.setSelectedId (1, juce::dontSendNotification);
        compareBox.onChange = [this] { compareDirty = true; repaint(); };

        addAndMakeVisible (status);
        status.setColour (juce::Label::textColourId, ui::textDim);
        status.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (hint);
        hint.setColour (juce::Label::textColourId, ui::textDim);
        hint.setText ("Up and down keys move through the list. With AUDITION on, each file you select replaces the cab blocks' sound; nothing in your blocks changes.",
                      juce::dontSendNotification);

        addButton.onClick = [this] { chooseFolder(); };
        rescanButton.onClick = [this] { library.rescan(); };
        removeButton.onClick = [this] { removeSelectedFolder(); };
        prevButton.onClick = [this] { step (-1); };
        nextButton.onClick = [this] { step (1); };
        favButton.onClick = [this]
        {
            if (auto* e = selectedEntry()) { library.setFavourite (e->path, ! library.isFavourite (e->path)); }
        };
        hideButton.onClick = [this]
        {
            if (auto* e = selectedEntry()) { library.setHidden (e->path, ! library.isHidden (e->path)); }
        };
        auditionButton.onClick = [this]
        {
            processor.setAuditionActive (auditionButton.getToggleState());
            if (auditionButton.getToggleState())
                loadAuditionForSelection();
        };
        juce::TextButton* sends[] = { &sendA, &sendB, &sendC, &sendD };
        for (int b = 0; b < kCabBlocks; ++b)
            sends[b]->onClick = [this, b]
            {
                if (auto* e = selectedEntry())
                {
                    processor.sendIRToBlock (juce::File (e->path), b);
                    if (onSent) onSent();
                }
            };

        favButton.setTooltip ("Mark or unmark this file as a favourite.");
        hideButton.setTooltip ("Hide this file from the list (Show hidden brings it back).");
        hideDupes.setTooltip ("Hide files whose curve is within 1 dB of an earlier file. This compares the average frequency curve only, so two files can still feel different.");
        auditionButton.setTooltip ("Hear the selected file in place of your cab blocks, with your amp's signal. Your blocks are not changed.");
        sendA.setTooltip ("Load this file into cab block A: sets it to IR file and switches it on.");

        setSize (1100, 600);
        updateButtons();
        if (library.getRoots().size() > 0)
            library.rescan();
        startTimerHz (4);
    }

    ~IrLibraryPanel() override
    {
        stopTimer();
        processor.setAuditionActive (false);
    }

    // Used by the editor's tests and screenshots: poll the scan, and pick a row.
    void refreshNow() { timerCallback(); }
    void selectRowNow (int row) { list.selectRow (row); }
    bool isScanningNow() const { return library.isScanning(); }
    int  numRows() const { return (int) view.size(); }
    void setFolderForTest (const juce::String& key) { folderKey = key; rebuildView(); }

    void visibilityChanged() override
    {
        if (! isShowing() && auditionButton.getToggleState())
        {
            auditionButton.setToggleState (false, juce::dontSendNotification);
            processor.setAuditionActive (false);
        }
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (ui::background);
        g.setColour (ui::panel);
        g.fillRoundedRectangle (plotArea.toFloat().expanded (0.0f, 0.0f), 6.0f);
        drawAnalysis (g);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (10);
        auto top = r.removeFromTop (28);
        addButton.setBounds (top.removeFromLeft (110));
        top.removeFromLeft (6);
        removeButton.setBounds (top.removeFromLeft (110));
        top.removeFromLeft (6);
        rescanButton.setBounds (top.removeFromLeft (80));
        top.removeFromLeft (10);
        search.setBounds (top.removeFromLeft (220));
        top.removeFromLeft (10);
        favOnly.setBounds (top.removeFromLeft (110));
        hideDupes.setBounds (top.removeFromLeft (140));
        showHidden.setBounds (top.removeFromLeft (120));
        status.setBounds (top);

        r.removeFromTop (8);
        auto bottom = r.removeFromBottom (58);
        auto buttons = bottom.removeFromTop (30);
        hint.setBounds (bottom);
        prevButton.setBounds (buttons.removeFromLeft (60));
        buttons.removeFromLeft (4);
        nextButton.setBounds (buttons.removeFromLeft (60));
        buttons.removeFromLeft (14);
        auditionButton.setBounds (buttons.removeFromLeft (120));
        buttons.removeFromLeft (14);
        favButton.setBounds (buttons.removeFromLeft (90));
        buttons.removeFromLeft (4);
        hideButton.setBounds (buttons.removeFromLeft (90));
        for (auto* b : { &sendD, &sendC, &sendB, &sendA })
        {
            b->setBounds (buttons.removeFromRight (86));
            buttons.removeFromRight (4);
        }

        r.removeFromBottom (6);
        tree.setBounds (r.removeFromLeft (230));
        r.removeFromLeft (8);
        list.setBounds (r.removeFromLeft (400));
        r.removeFromLeft (8);
        compareBox.setBounds (r.removeFromTop (26));
        r.removeFromTop (6);
        plotArea = r.removeFromTop (230);
        r.removeFromTop (8);
        textArea = r;
    }

private:
    //------------------------------------------------------------------ folder tree
    struct FolderItem : public juce::TreeViewItem
    {
        FolderItem (IrLibraryPanel& o, juce::String l, juce::String k, int n) : owner (o), label (std::move (l)), key (std::move (k)), count (n) {}
        bool mightContainSubItems() override { return getNumSubItems() > 0; }
        juce::String getUniqueName() const override { return key; }
        int getItemHeight() const override { return 22; }
        void paintItem (juce::Graphics& g, int w, int h) override
        {
            if (isSelected())
            {
                g.setColour (ui::accent.withAlpha (0.25f));
                g.fillRect (0, 0, w, h);
            }
            g.setColour (isSelected() ? ui::text : ui::text.withAlpha (0.85f));
            g.setFont (13.0f);
            g.drawText (label, 4, 0, w - 50, h, juce::Justification::centredLeft, true);
            g.setColour (ui::textDim);
            g.drawText (juce::String (count), w - 46, 0, 42, h, juce::Justification::centredRight);
        }
        void itemSelectionChanged (bool nowSelected) override { if (nowSelected) owner.setFolderFilter (key); }
        IrLibraryPanel& owner;
        juce::String label, key;
        int count;
    };

    void setFolderFilter (const juce::String& key) { folderKey = key; rebuildView(); }

    void rebuildTree()
    {
        // Count files per folder and find the subfolders that hold any
        std::map<juce::String, int> counts;
        std::map<juce::String, std::set<juce::String>> children;
        const auto roots = library.getRoots();
        int favCount = 0;
        for (auto& e : entries)
        {
            if (library.isFavourite (e.path)) ++favCount;
            for (auto& root : roots)
            {
                if (! e.path.startsWith (root + juce::File::getSeparatorString()))
                    continue;
                juce::File dir = juce::File (e.path).getParentDirectory();
                juce::File child;
                bool first = true;
                while (true)
                {
                    const auto dp = dir.getFullPathName();
                    counts[dp]++;
                    if (! first) children[dp].insert (child.getFullPathName());
                    first = false;
                    if (dp == root) break;
                    child = dir;
                    dir = dir.getParentDirectory();
                }
                break;
            }
        }

        juce::String signature = juce::String ((int) entries.size() > 0 ? 1 : 0);
        for (auto& kv : counts) signature << kv.first << ":" << kv.second << ";";
        signature << "fav" << favCount << "all" << (int) entries.size();
        if (signature == treeSignature)
            return;
        treeSignature = signature;

        std::function<void (FolderItem&, const juce::String&)> addChildren = [&] (FolderItem& parent, const juce::String& dir)
        {
            auto it = children.find (dir);
            if (it == children.end()) return;
            for (auto& childPath : it->second)
            {
                auto* item = new FolderItem (*this, juce::File (childPath).getFileName(), childPath, counts[childPath]);
                parent.addSubItem (item);
                addChildren (*item, childPath);
            }
        };

        auto* root = new FolderItem (*this, "root", "root", 0);
        root->addSubItem (new FolderItem (*this, "All IRs", "*all*", (int) entries.size()));
        root->addSubItem (new FolderItem (*this, "Favourites", "*fav*", favCount));
        for (auto& r : roots)
        {
            auto* item = new FolderItem (*this, juce::File (r).getFileName(), r, counts[r]);
            root->addSubItem (item);
            addChildren (*item, r);
        }
        tree.setRootItem (nullptr);
        treeRoot.reset (root);
        tree.setRootItem (treeRoot.get());
        for (int i = 0; i < tree.getNumRowsInTree(); ++i)
            if (auto* it = dynamic_cast<FolderItem*> (tree.getItemOnRow (i)))
                if (it->key == folderKey) { it->setSelected (true, true, juce::dontSendNotification); break; }
    }

    void removeSelectedFolder()
    {
        // Only a folder added with ADD FOLDER (a top level one) can be removed
        for (auto& r : library.getRoots())
            if (r == folderKey)
            {
                library.removeRoot (r);
                folderKey = "*all*";
                treeSignature = {};
                return;
            }
        say ("Select one of the folders you added (the top level of a branch), then press REMOVE FOLDER.");
    }

    void chooseFolder()
    {
        chooser = std::make_unique<juce::FileChooser> ("Choose a folder of impulse responses (WAV or AIFF; subfolders are included)", juce::File());
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                              [safe = juce::Component::SafePointer<IrLibraryPanel> (this)] (const juce::FileChooser& fc)
                              {
                                  if (safe != nullptr && fc.getResult().isDirectory())
                                      safe->library.addRoot (fc.getResult());
                              });
    }

    //------------------------------------------------------------------ list
    int getNumRows() override { return (int) view.size(); }

    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override
    {
        if (row < 0 || row >= (int) view.size()) return;
        const auto& e = entries[(size_t) view[(size_t) row]];
        if (selected)
        {
            g.setColour (ui::accent.withAlpha (0.25f));
            g.fillRect (0, 0, w, h);
        }
        const bool dup = e.dupOf >= 0, hid = library.isHidden (e.path);
        const float alpha = (dup || hid) ? 0.5f : 1.0f;

        g.setColour ((library.isFavourite (e.path) ? ui::accent : ui::textDim).withAlpha (library.isFavourite (e.path) ? 1.0f : 0.5f));
        g.setFont (14.0f);
        g.drawText (library.isFavourite (e.path) ? juce::String::fromUTF8 ("\xe2\x98\x85") : juce::String::fromUTF8 ("\xc2\xb7"), 2, 0, 16, h, juce::Justification::centred);

        g.setColour (ui::text.withAlpha (alpha));
        g.setFont (13.0f);
        g.drawText ((dup ? juce::String::fromUTF8 ("\xe2\x89\x88 ") : juce::String()) + e.name(), 20, 0, w - 20 - 150, h, juce::Justification::centredLeft, true);

        // A small picture of the curve
        const juce::Rectangle<float> box ((float) w - 146.0f, 4.0f, 78.0f, (float) h - 8.0f);
        juce::Path path;
        for (int i = 0; i < irlib::kGrid; i += 2)
        {
            const float x = box.getX() + box.getWidth() * (float) i / (irlib::kGrid - 1);
            const float y = box.getCentreY() - juce::jlimit (-12.0f, 12.0f, e.curve[(size_t) i]) / 12.0f * box.getHeight() * 0.5f;
            if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
        }
        g.setColour (ui::accent.withAlpha (0.8f * alpha));
        g.strokePath (path, juce::PathStrokeType (1.2f));

        g.setColour (ui::textDim.withAlpha (alpha));
        g.setFont (11.0f);
        g.drawText (juce::String (juce::roundToInt (e.lengthMs)) + " ms", w - 66, 0, 62, h, juce::Justification::centredRight);
    }

    void selectedRowsChanged (int) override
    {
        if (suppressSelect) return;
        selectionChanged();
    }

    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override
    {
        if (row >= 0 && row < (int) view.size())
        {
            auditionButton.setToggleState (true, juce::dontSendNotification);
            processor.setAuditionActive (true);
            loadAuditionForSelection();
        }
    }

    void selectionChanged()
    {
        if (auto* e = selectedEntry())
            selectedPath = e->path;
        compareDirty = true;
        updateButtons();
        if (auditionButton.getToggleState())
            loadAuditionForSelection();
        repaint();
    }

    const irlib::Entry* selectedEntry() const
    {
        const int row = list.getSelectedRow();
        if (row < 0 || row >= (int) view.size()) return nullptr;
        return &entries[(size_t) view[(size_t) row]];
    }

    void loadAuditionForSelection()
    {
        if (auto* e = selectedEntry())
            processor.auditionLoad (juce::File (e->path));
    }

    void step (int delta)
    {
        const int n = (int) view.size();
        if (n == 0) return;
        const int row = juce::jlimit (0, n - 1, list.getSelectedRow() < 0 ? 0 : list.getSelectedRow() + delta);
        list.selectRow (row);
        list.scrollToEnsureRowIsOnscreen (row);
    }

    void updateButtons()
    {
        const auto* e = selectedEntry();
        const bool has = e != nullptr;
        for (auto* b : { &favButton, &hideButton, &sendA, &sendB, &sendC, &sendD })
            b->setEnabled (has);
        favButton.setButtonText (has && library.isFavourite (e->path) ? "UNFAVOURITE" : "FAVOURITE");
        hideButton.setButtonText (has && library.isHidden (e->path) ? "UNHIDE" : "HIDE");
    }

    void rebuildView()
    {
        const auto query = search.getText().trim().toLowerCase();
        view.clear();
        for (int i = 0; i < (int) entries.size(); ++i)
        {
            const auto& e = entries[(size_t) i];
            if (! showHidden.getToggleState() && library.isHidden (e.path)) continue;
            if (hideDupes.getToggleState() && e.dupOf >= 0) continue;
            if (favOnly.getToggleState() && ! library.isFavourite (e.path)) continue;
            if (folderKey == "*fav*" && ! library.isFavourite (e.path)) continue;
            if (folderKey != "*all*" && folderKey != "*fav*"
                && ! e.path.startsWith (folderKey + juce::File::getSeparatorString())) continue;
            if (query.isNotEmpty() && ! e.path.toLowerCase().contains (query)) continue;
            view.push_back (i);
        }
        suppressSelect = true;
        list.updateContent();
        int keep = -1;
        for (int r = 0; r < (int) view.size(); ++r)
            if (entries[(size_t) view[(size_t) r]].path == selectedPath) { keep = r; break; }
        if (keep >= 0) list.selectRow (keep, true, false); else list.deselectAllRows();
        suppressSelect = false;
        list.repaint();
        updateButtons();
        compareDirty = true;
        repaint();
    }

    void say (const juce::String& t) { status.setText (t, juce::dontSendNotification); }

    //------------------------------------------------------------------ analysis display
    void timerCallback() override
    {
        const int v = library.getVersion();
        if (v != seenVersion)
        {
            seenVersion = v;
            entries = library.getEntries();
            rebuildTree();
            rebuildView();
        }
        if (library.isScanning())
            say ("Scanning " + juce::String (library.scannedCount()) + " of " + juce::String (library.totalCount()) + "...");
        else if (library.getRoots().isEmpty())
            say ("Press ADD FOLDER to choose a folder of IR files.");
        else
        {
            juce::String t = juce::String ((int) entries.size()) + " IR files";
            if (library.skippedCount() > 0) t << ", " << library.skippedCount() << " skipped";
            say (t);
        }
        const int irv = processor.getIRVersion();
        if (irv != seenIrVersion) { seenIrVersion = irv; compareDirty = true; repaint(); }
        if (compareDirty) updateCompare();
    }

    void updateCompare()
    {
        compareDirty = false;
        compareValid = false;
        compareName = {};
        const int id = compareBox.getSelectedId();
        const auto* e = selectedEntry();
        if (e == nullptr || id == 6) { repaint(); return; }
        if (id == 1)
        {
            if (e->closest >= 0 && e->closest < (int) StarterAmpProcessor::builtinSpeakerCurves().size())
            {
                compare = StarterAmpProcessor::builtinSpeakerCurves()[(size_t) e->closest];
                compareName = speakerNames[e->closest];
                compareValid = true;
            }
        }
        else
        {
            const int slot = id - 2;
            const auto snap = processor.getIRSnapshot (slot);
            const float* ch[1] = { snap.data() };
            if (! snap.empty() && irlib::analyseSamples (ch, 1, (int) snap.size(), StarterAmpProcessor::kIRAnalysisRate, compare.data()))
            {
                compareName = "cab block " + cabLetter (slot);
                compareValid = true;
            }
        }
        repaint();
    }

    static juce::String signedDb (float v) { return (v >= 0 ? "+" : "") + juce::String (v, 1) + " dB"; }

    void drawAnalysis (juce::Graphics& g)
    {
        const auto* e = selectedEntry();
        auto area = plotArea.reduced (8, 6);
        if (e == nullptr)
        {
            g.setColour (ui::textDim);
            g.setFont (13.0f);
            g.drawFittedText ("Select a file to see its frequency curve.\nThe curve is smoothed to 1/6 octave and centred on the 500 Hz to 2 kHz level.",
                              plotArea.reduced (20), juce::Justification::centred, 3);
            return;
        }

        auto xOf = [&] (float hz) { return (float) area.getX() + (float) area.getWidth() * std::log (hz / 50.0f) / std::log (9000.0f / 50.0f); };
        auto yOf = [&] (float db) { return (float) area.getCentreY() - juce::jlimit (-22.0f, 22.0f, db) / 22.0f * (float) area.getHeight() * 0.5f; };

        g.setFont (10.0f);
        for (float hz : { 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f })
        {
            g.setColour (ui::panelEdge);
            g.drawVerticalLine ((int) xOf (hz), (float) area.getY(), (float) area.getBottom());
            g.setColour (ui::textDim);
            g.drawText (hz >= 1000.0f ? juce::String (hz / 1000.0f, 0) + "k" : juce::String ((int) hz), (int) xOf (hz) - 16, area.getBottom() - 12, 32, 12, juce::Justification::centred);
        }
        for (float db : { -20.0f, -10.0f, 0.0f, 10.0f, 20.0f })
        {
            g.setColour (db == 0.0f ? ui::panelEdge.brighter (0.3f) : ui::panelEdge);
            g.drawHorizontalLine ((int) yOf (db), (float) area.getX(), (float) area.getRight());
            g.setColour (ui::textDim);
            g.drawText (juce::String ((int) db), area.getX() + 2, (int) yOf (db) - 10, 26, 10, juce::Justification::centredLeft);
        }

        auto plot = [&] (const irlib::Curve& c, juce::Colour col, float thick)
        {
            juce::Path p;
            for (int i = 0; i < irlib::kGrid; ++i)
            {
                const float x = xOf (irlib::gridHz()[(size_t) i]), y = yOf (c[(size_t) i]);
                if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
            }
            g.setColour (col);
            g.strokePath (p, juce::PathStrokeType (thick));
        };
        if (compareValid)
            plot (compare, ui::colourB, 1.5f);
        plot (e->curve, ui::accent, 2.2f);

        g.setFont (11.0f);
        g.setColour (ui::accent);
        g.drawText ("This file", area.getRight() - 160, area.getY() + 2, 156, 12, juce::Justification::centredRight);
        if (compareValid)
        {
            g.setColour (ui::colourB);
            g.drawText (compareName, area.getRight() - 160, area.getY() + 14, 156, 12, juce::Justification::centredRight);
        }

        // Text under the plot
        auto t = textArea;
        const auto s = irlib::summarise (e->curve.data());
        auto line = [&] (const juce::String& text, juce::Colour c, float size = 12.5f)
        {
            g.setColour (c);
            g.setFont (size);
            auto row = t.removeFromTop ((int) (size + 6.0f));
            g.drawText (text, row, juce::Justification::centredLeft, true);
        };
        line (e->name(), ui::text, 14.0f);
        line (juce::String (juce::roundToInt (e->lengthMs)) + " ms, " + juce::String (e->sampleRate / 1000.0f, 1) + " kHz, "
              + (e->channels > 1 ? "stereo" : "mono") + ", in " + juce::File (e->path).getParentDirectory().getFileName(), ui::textDim, 11.5f);
        line ("Low 80-250 Hz " + signedDb (s.low) + "   Low-mid 250-500 Hz " + signedDb (s.lowMid), ui::text);
        line ("Upper-mid 2-4 kHz " + signedDb (s.upperMid) + "   Presence 4-6.5 kHz " + signedDb (s.presence), ui::text);
        line ("Low resonance near " + juce::String (juce::roundToInt (s.lowPeakHz)) + " Hz (" + signedDb (s.lowPeakDb) + ").  "
              + (s.topCutHz > 0.0f ? "Top rolls off near " + juce::String (s.topCutHz / 1000.0f, 1) + " kHz." : "No top roll-off below 9 kHz."), ui::text);

        if (e->closest >= 0)
        {
            const auto nm = speakerNames[e->closest];
            juce::String verdict = e->closestDist < 1.25f ? "Same sound as " + nm
                                 : e->closestDist < 2.4f  ? "Close to " + nm
                                                          : "Different from every built-in speaker (nearest: " + nm + ")";
            line (verdict + " (" + juce::String (e->closestDist, 1) + " dB apart)", e->closestDist >= 2.4f ? ui::colourD : ui::text);
        }
        if (compareValid && compareBox.getSelectedId() != 1)
            line ("Distance to " + compareName + ": " + juce::String (irlib::distanceDb (e->curve.data(), compare.data()), 1) + " dB", ui::text);
        if (e->dupOf >= 0 && e->dupOf < (int) entries.size())
            line (juce::String::fromUTF8 ("\xe2\x89\x88 ") + "Within 1 dB of " + entries[(size_t) e->dupOf].name(), ui::colourC);
        else if (e->dupCount > 0)
            line (juce::String (e->dupCount) + (e->dupCount == 1 ? " other file" : " other files") + " in your library sound the same as this one", ui::colourC);
    }

    //------------------------------------------------------------------
    StarterAmpProcessor& processor;
    std::function<void()> onSent;
    irlib::Library library;
    juce::StringArray speakerNames;

    std::vector<irlib::Entry> entries;
    std::vector<int> view;
    juce::String folderKey { "*all*" }, selectedPath, treeSignature;
    int seenVersion = -1, seenIrVersion = -1;
    bool suppressSelect = false, compareDirty = true, compareValid = false;
    irlib::Curve compare {};
    juce::String compareName;

    juce::TextButton addButton { "ADD FOLDER" }, removeButton { "REMOVE FOLDER" }, rescanButton { "RESCAN" },
                     prevButton { "PREV" }, nextButton { "NEXT" }, favButton { "FAVOURITE" }, hideButton { "HIDE" },
                     sendA { "SEND TO A" }, sendB { "SEND TO B" }, sendC { "SEND TO C" }, sendD { "SEND TO D" };
    juce::TextButton auditionButton { "AUDITION" }, favOnly { "FAVOURITES ONLY" }, hideDupes { "HIDE DUPLICATES" }, showHidden { "SHOW HIDDEN" };
    juce::TextEditor search;
    juce::TreeView tree;
    std::unique_ptr<FolderItem> treeRoot;
    juce::ListBox list { {}, this };
    juce::ComboBox compareBox;
    juce::Label status, hint;
    juce::Rectangle<int> plotArea, textArea;
    std::unique_ptr<juce::FileChooser> chooser;
};
