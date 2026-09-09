#ifndef PRESENTATION_MAIN_WINDOW_RIBBON_WINDOW_STATE_H
#define PRESENTATION_MAIN_WINDOW_RIBBON_WINDOW_STATE_H

class QStackedWidget;

class SARibbonBar;
class SARibbonCategory;
class SARibbonContextCategory;

/** Runtime references and mode state for the main modeling ribbon. */
class RibbonWindowState {
protected:
    QStackedWidget* toolbarModeStack_ = nullptr;
    SARibbonBar* appRibbonBar_ = nullptr;
    SARibbonCategory* ribbonCatView_ = nullptr;
    SARibbonCategory* ribbonCatModel_ = nullptr;
    SARibbonContextCategory* sketchRibbonContext_ = nullptr;
    SARibbonCategory* sketchRibbonCategory_ = nullptr;
    bool inSketchEnvironment_ = false;
    int savedNormalTabIndex_ = 0;
};

#endif // PRESENTATION_MAIN_WINDOW_RIBBON_WINDOW_STATE_H
