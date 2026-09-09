#ifndef PRESENTATION_MAIN_WINDOW_DOCUMENT_WINDOW_STATE_H
#define PRESENTATION_MAIN_WINDOW_DOCUMENT_WINDOW_STATE_H

#include <QList>
#include <QString>
#include <QTimer>
#include <QVector>

class QGroupBox;
class QMenu;
class QPushButton;
class QToolButton;

/** Document/session state kept by the main-window presentation boundary. */
class DocumentWindowState {
protected:
    QString currentDocumentPath_;
    bool documentModified_ = false;
    bool isLoadingDocument_ = false;

    QList<QString> recentFiles_;
    QTimer* autoSaveTimer_ = nullptr;

    QGroupBox* recentFilesGroup_ = nullptr;
    QVector<QPushButton*> recentFileButtons_;
    QToolButton* openRecentMenuButton_ = nullptr;
    QMenu* openRecentMenu_ = nullptr;
};

#endif // PRESENTATION_MAIN_WINDOW_DOCUMENT_WINDOW_STATE_H
