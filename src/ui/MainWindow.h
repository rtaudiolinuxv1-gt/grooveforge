#pragma once

#include <vector>

#include <QMainWindow>
#include <QPointer>
#include <QString>

#include "app/GrooveController.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QGridLayout;
class QLabel;
class QLayout;
class QLineEdit;
class QMediaPlayer;
class QPushButton;
class QScrollArea;
class QSlider;
class QSpinBox;
class QTimer;
class QVBoxLayout;
class QWidget;
class QResizeEvent;
QT_END_NAMESPACE

namespace groove {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(GrooveController* controller, QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    struct InstrumentWidgets {
        QLineEdit* nameEdit = nullptr;
        QComboBox* roleCombo = nullptr;
        QSlider* densitySlider = nullptr;
        QCheckBox* sampleCheck = nullptr;
        QCheckBox* soundfontCheck = nullptr;
        QCheckBox* midiCheck = nullptr;
        QSpinBox* midiChannelSpin = nullptr;
        QSpinBox* soundfontChannelSpin = nullptr;
        QSpinBox* soundfontBankSpin = nullptr;
        QSpinBox* soundfontProgramSpin = nullptr;
        QComboBox* soundfontPresetCombo = nullptr;
        QPushButton* loadSampleButton = nullptr;
        QPushButton* clearSampleButton = nullptr;
        QLabel* sampleLabel = nullptr;
    };

    void buildUi();
    void rebuildInstrumentEditors();
    void rebuildStepGrid();
    void clearLayout(QLayout* layout);
    void updateStepGridButtonSizing();
    void refreshGridGeometry();
    void scheduleGridGeometryRefresh();
    void setSelectedStep(int instrumentIndex, int absoluteStepIndex);
    bool hasSelectedVisibleStep(const GrooveScene& scene) const;
    void updateVirtualKeyboard();
    void applySelectedStepNote(int midiNote);
    void shiftSelectedKeyboardOctave(int delta);
    void populateRoleCombo(QComboBox* combo) const;
    InstrumentRole roleFromCombo(const QComboBox* combo) const;
    int roleComboIndex(InstrumentRole role) const;
    void refreshFromScene();
    void refreshStepHighlight();
    void updateStatus();
    void syncActiveBarToTransport();
    void saveProject();
    void loadProject();
    void loadSampleForInstrument(int instrumentIndex);
    void editInstrumentDefaults(int instrumentIndex);
    void editStepParameters(int instrumentIndex, int stepIndex);
    void setPreviewFile(const QString& path);
    void seekPreview(qint64 deltaMs);
    void loadSoundfontFile();
    void renderBarsToWav();
    void renderSecondsToWav();
    void startRecordingToFile(AudioFileFormat format);
    int currentEditBarIndex() const;
    QPushButton* makeStepButton(int instrumentIndex, int stepIndex);
    QString stepButtonStyle(bool active, bool currentStep, bool locked, bool selected) const;
    QString sampleLabelText(const std::string& path) const;
    QString soundfontLabelText(const std::string& path) const;

    GrooveController* controller_ = nullptr;
    QPointer<QTimer> timer_;
    QLabel* statusLabel_ = nullptr;
    QLabel* transportLabel_ = nullptr;
    QPushButton* playButton_ = nullptr;
    QPushButton* connectOutputsButton_ = nullptr;
    QPushButton* recordWavButton_ = nullptr;
    QPushButton* recordFlacButton_ = nullptr;
    QPushButton* stopRecordButton_ = nullptr;
    QPushButton* renderBarsWavButton_ = nullptr;
    QPushButton* renderSecondsWavButton_ = nullptr;
    QPushButton* previewPlayButton_ = nullptr;
    QPushButton* previewStopButton_ = nullptr;
    QPushButton* previewForwardButton_ = nullptr;
    QPushButton* previewRewindButton_ = nullptr;
    QPushButton* loadSoundfontButton_ = nullptr;
    QPushButton* clearSoundfontButton_ = nullptr;
    QPushButton* addInstrumentButton_ = nullptr;
    QLabel* previewFileLabel_ = nullptr;
    QLabel* soundfontLabel_ = nullptr;
    QLineEdit* newInstrumentNameEdit_ = nullptr;
    QComboBox* newInstrumentRoleCombo_ = nullptr;
    QSpinBox* bpmSpin_ = nullptr;
    QSpinBox* patternBarsSpin_ = nullptr;
    QSpinBox* stepsPerBarSpin_ = nullptr;
    QSpinBox* editBarSpin_ = nullptr;
    QSpinBox* repeatSpin_ = nullptr;
    QSpinBox* exportBarsSpin_ = nullptr;
    QDoubleSpinBox* exportSecondsSpin_ = nullptr;
    QCheckBox* mutationEnabledCheck_ = nullptr;
    QSlider* swingSlider_ = nullptr;
    QSlider* mutationSlider_ = nullptr;
    QWidget* instrumentEditorWidget_ = nullptr;
    QVBoxLayout* instrumentEditorLayout_ = nullptr;
    QScrollArea* stepScrollArea_ = nullptr;
    QWidget* stepGridWidget_ = nullptr;
    QGridLayout* stepGridLayout_ = nullptr;
    QGroupBox* stepKeyboardBox_ = nullptr;
    QLabel* selectedStepLabel_ = nullptr;
    QPushButton* octaveDownButton_ = nullptr;
    QPushButton* octaveUpButton_ = nullptr;
    std::vector<QPushButton*> keyboardNoteButtons_;
    QMediaPlayer* previewPlayer_ = nullptr;
    QString previewPath_;
    QString lastMessage_;
    std::vector<InstrumentWidgets> instrumentWidgets_;
    std::vector<QLabel*> stepRowLabels_;
    std::vector<std::vector<QPushButton*>> stepButtons_;
    int selectedInstrumentIndex_ = -1;
    int selectedAbsoluteStepIndex_ = -1;
    int selectedKeyboardOctave_ = 4;
};

}  // namespace groove
