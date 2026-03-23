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
class QGridLayout;
class QLabel;
class QLayout;
class QLineEdit;
class QPushButton;
class QSlider;
class QSpinBox;
class QTimer;
class QVBoxLayout;
class QWidget;
QT_END_NAMESPACE

namespace groove {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(GrooveController* controller, QWidget* parent = nullptr);

private:
    struct InstrumentWidgets {
        QLineEdit* nameEdit = nullptr;
        QComboBox* roleCombo = nullptr;
        QSlider* densitySlider = nullptr;
        QCheckBox* synthCheck = nullptr;
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
    void populateRoleCombo(QComboBox* combo) const;
    InstrumentRole roleFromCombo(const QComboBox* combo) const;
    int roleComboIndex(InstrumentRole role) const;
    void refreshFromScene();
    void refreshStepHighlight();
    void updateStatus();
    void loadSampleForInstrument(int instrumentIndex);
    void loadSoundfontFile();
    void renderBarsToWav();
    void renderSecondsToWav();
    void startRecordingToFile(AudioFileFormat format);
    int currentEditBarIndex() const;
    QPushButton* makeStepButton(int instrumentIndex, int stepIndex);
    QString stepButtonStyle(bool active, bool currentStep) const;
    QString sampleLabelText(const std::string& path) const;
    QString soundfontLabelText(const std::string& path) const;

    GrooveController* controller_ = nullptr;
    QPointer<QTimer> timer_;
    QLabel* statusLabel_ = nullptr;
    QLabel* transportLabel_ = nullptr;
    QPushButton* playButton_ = nullptr;
    QPushButton* recordWavButton_ = nullptr;
    QPushButton* recordFlacButton_ = nullptr;
    QPushButton* stopRecordButton_ = nullptr;
    QPushButton* renderBarsWavButton_ = nullptr;
    QPushButton* renderSecondsWavButton_ = nullptr;
    QPushButton* loadSoundfontButton_ = nullptr;
    QPushButton* clearSoundfontButton_ = nullptr;
    QPushButton* addInstrumentButton_ = nullptr;
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
    QWidget* stepGridWidget_ = nullptr;
    QGridLayout* stepGridLayout_ = nullptr;
    QString lastMessage_;
    std::vector<InstrumentWidgets> instrumentWidgets_;
    std::vector<QLabel*> stepRowLabels_;
    std::vector<std::vector<QPushButton*>> stepButtons_;
};

}  // namespace groove
