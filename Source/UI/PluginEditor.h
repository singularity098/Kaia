#pragma once

#include "LookAndFeel.h"
#include "PadGrid.h"
#include "PadEditor.h"
#include "ProgramBrowser.h"
#include "LibraryBrowser.h"
#include "FileBrowser.h"
#include "SettingsPanel.h"

namespace kaia
{

class KaiaEditor : public juce::AudioProcessorEditor,
                       public juce::DragAndDropContainer,
                       private juce::ChangeListener,
                       private juce::Timer
{
public:
    explicit KaiaEditor (KaiaProcessor&);
    ~KaiaEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    // A plugin wrapper's window resized by the host through X alone (Carla 2.5 resizes a VST3
    // editor's window and tells the editor nothing): take the window's size
    void parentSizeChanged() override;

    // Re-reads the theme, tint and highlight colour from the settings and restyles the whole window
    void themeChanged();

    // The whole interface lives in `content`, laid out at a logical size and scaled to fill the
    // window: big windows (full screen) zoom it up, windows below the minimum layout shrink it
    // instead of cutting it off. kDesignWidth/Height is the default size, kMinLayout* the smallest
    // logical size the layout works at.
    static constexpr int kDesignWidth = 1120, kDesignHeight = 720;
    static constexpr int kMinLayoutWidth = 900, kMinLayoutHeight = 600;
    static float zoomForSize (int width, int height, bool zoomWithWindow);
    float getZoom() const noexcept   { return zoom; }
    juce::Component& getContent() noexcept   { return content; }

private:
    // Holds every control; scaled by an AffineTransform (see zoomForSize)
    struct Content : juce::Component, juce::FileDragAndDropTarget
    {
        explicit Content (KaiaEditor& e) : owner (e) {}
        void paint (juce::Graphics& g) override                 { owner.paintContent (g); }
        void paintOverChildren (juce::Graphics& g) override     { owner.paintContentOver (g); }
        void resized() override                                 { owner.layoutContent(); }
        bool isInterestedInFileDrag (const juce::StringArray& f) override         { return owner.isInterestedInFileDrag (f); }
        void fileDragMove (const juce::StringArray& f, int x, int y) override     { owner.fileDragMove (f, x, y); }
        void fileDragExit (const juce::StringArray& f) override                   { owner.fileDragExit (f); }
        void filesDropped (const juce::StringArray& f, int x, int y) override     { owner.filesDropped (f, x, y); }
        KaiaEditor& owner;
    };

    void paintContent (juce::Graphics&);
    void paintContentOver (juce::Graphics&);
    void layoutContent();
    bool isInterestedInFileDrag (const juce::StringArray& files);
    void fileDragMove (const juce::StringArray& files, int x, int y);
    void fileDragExit (const juce::StringArray& files);
    void filesDropped (const juce::StringArray& files, int x, int y);

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    void refreshAll();
    void showOpenMenu();
    void showRoutingMenu();
    void showPadMenu (int pad, juce::Point<int> screenPos);
    void chooseFiles (bool folders);
    void chooseSamplesForPad (int pad, KaiaProcessor::AssignMode mode, int layer);
    void chooseRelinkFolder();
    enum class Panel { PadEditor, Files, Library, Programs, Settings };
    void showPanel (Panel panel);
    void togglePanel (Panel panel)   { showPanel (currentPanel == panel ? Panel::PadEditor : panel); }
    void chooseLibraryFolder();
    void chooseFilesFolder();
    void savePresetDialog();
    void addRecent (const juce::File& f);
    juce::StringArray getRecents();
    juce::File lastFolder();

    KaiaProcessor& processor;
    KaiaLookAndFeel lookAndFeel;
    Content content { *this };
    float zoom = 1.0f;
    bool sizeRestored = false;     // resized() remembers the size only once the saved one is applied
    juce::TooltipWindow tooltips { &content, 600 };     // inside content, so tips zoom with it
    std::unique_ptr<juce::Drawable> wordmark;      // "KAIA" in the header logo

    juce::TextButton openButton { "Open" }, browseButton { "Programs" }, prevButton { "<" }, nextButton { ">" };
    juce::TextButton panicButton { "Stop" }, relinkButton { "Relink..." }, routingButton { "Outputs" };
    juce::TextButton filesButton { "Files" }, libraryButton { "Library" }, settingsButton { "Settings" };
    juce::Label kitLabel, formatLabel, statusLabel, voicesLabel;
    juce::ImageButton coverButton;                 // the open expansion's artwork
    juce::String shownCoverKey;
    juce::HyperlinkButton logoLink { {}, juce::URL ("https://github.com/singularity098/kaia") };   // over the painted logo
    juce::Slider masterSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment masterAttachment;

    // Linux: re-sets the window's cursor whenever the mouse enters a component. JUCE skips the update
    // when it thinks the normal cursor is already showing, and an embedded window that never set one
    // shows the host's (Reaper's resize arrow when entered from the side). Windows and macOS force
    // the update on enter themselves. Needs the X11 cursor fix in cmake/PatchJuce.cmake.
    struct CursorRefresher : juce::MouseListener
    {
        void mouseEnter (const juce::MouseEvent& e) override
        {
            if (e.source.isMouse())
            {
                auto source = e.source;
                source.forceMouseCursorUpdate();
            }
        }
    } cursorRefresher;

    PadGrid padGrid;
    PadEditor padEditor;
    ProgramBrowser browser;
    LibraryBrowser library;
    FileBrowser files;
    SettingsPanel settingsPanel;
    Panel currentPanel = Panel::PadEditor;

    std::unique_ptr<juce::FileChooser> chooser;
    juce::SharedResourcePointer<SharedSettings> sharedSettings;
    juce::PropertiesFile& settings() { return sharedSettings->get(); }
    std::unique_ptr<juce::AlertWindow> presetDialog;
    std::unique_ptr<Pad> clipboard;
    juce::ComponentBoundsConstrainer constrainer;

    void updateCover();
    void loadWordmark();

    juce::Rectangle<int> headerArea, footerArea, progressArea, midiArea;
    juce::uint32 shownInputNote = 0;     // last MIDI note-on shown in the header
    float midiFlash = 0.0f;
    float shownProgress = 0.0f;
    bool wasLoading = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KaiaEditor)
};

}
