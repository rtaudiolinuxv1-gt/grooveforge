#include "ui/MainWindow.h"

#include <algorithm>

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace groove {

namespace {

struct GmPresetChoice {
    const char* name;
    int bank;
    int program;
};

const GmPresetChoice kGmPresetChoices[] = {
    {"Custom Bank/Program", -1, -1},
    {"Acoustic Grand Piano", 0, 0},
    {"Bright Piano", 0, 1},
    {"Honky-tonk Piano", 0, 3},
    {"Electric Piano", 0, 4},
    {"Drawbar Organ", 0, 16},
    {"Nylon Guitar", 0, 24},
    {"Clean Guitar", 0, 27},
    {"Acoustic Bass", 0, 32},
    {"Finger Bass", 0, 33},
    {"Fretless Bass", 0, 35},
    {"Strings", 0, 48},
    {"Choir", 0, 52},
    {"Trumpet", 0, 56},
    {"Alto Sax", 0, 65},
    {"Flute", 0, 73},
    {"Square Lead", 0, 80},
    {"Saw Lead", 0, 81},
    {"Warm Pad", 0, 89},
    {"Drum Kit", 128, 0},
};

int gmPresetIndexFor(int bank, int program) {
    constexpr int kPresetCount = static_cast<int>(sizeof(kGmPresetChoices) / sizeof(kGmPresetChoices[0]));
    for (int index = 0; index < kPresetCount; ++index) {
        if ((kGmPresetChoices[index].bank == bank) && (kGmPresetChoices[index].program == program)) {
            return index;
        }
    }
    return 0;
}

}  // namespace

MainWindow::MainWindow(GrooveController* controller, QWidget* parent)
    : QMainWindow(parent), controller_(controller) {
    buildUi();
    refreshFromScene();

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, [this]() {
        controller_->tickAutomation();
        refreshFromScene();
        refreshStepHighlight();
        updateStatus();
    });
    timer_->start(50);
}

void MainWindow::buildUi() {
    auto* central = new QWidget(this);
    auto* outer = new QVBoxLayout(central);
    outer->setSpacing(16);
    outer->setContentsMargins(20, 20, 20, 20);

    auto titleFont = QFont("DejaVu Sans", 20, QFont::Bold);
    auto labelFont = QFont("DejaVu Sans Mono", 10);
    setFont(labelFont);
    setWindowTitle("Groove Forge");
    resize(1560, 980);

    auto* header = new QHBoxLayout();
    auto* title = new QLabel("Groove Forge");
    title->setFont(titleFont);
    transportLabel_ = new QLabel("STOP");
    transportLabel_->setObjectName("transportLabel");
    statusLabel_ = new QLabel();
    statusLabel_->setWordWrap(true);
    header->addWidget(title, 1);
    header->addWidget(transportLabel_);
    outer->addLayout(header);
    outer->addWidget(statusLabel_);

    auto* tabs = new QTabWidget();
    auto* performanceTab = new QWidget();
    auto* performanceLayout = new QVBoxLayout(performanceTab);
    performanceLayout->setSpacing(16);
    auto* instrumentsTab = new QWidget();
    auto* instrumentsTabLayout = new QVBoxLayout(instrumentsTab);
    instrumentsTabLayout->setSpacing(16);
    auto* soundfontTab = new QWidget();
    auto* soundfontTabLayout = new QVBoxLayout(soundfontTab);
    soundfontTabLayout->setSpacing(16);
    auto* exportTab = new QWidget();
    auto* exportTabLayout = new QVBoxLayout(exportTab);
    exportTabLayout->setSpacing(16);

    auto* controlBox = new QGroupBox("Scene Controls");
    auto* controlLayout = new QGridLayout(controlBox);

    playButton_ = new QPushButton("Play");
    auto* regenerateButton = new QPushButton("Regenerate");
    auto* mutateButton = new QPushButton("Mutate Now");
    bpmSpin_ = new QSpinBox();
    bpmSpin_->setRange(40, 220);
    patternBarsSpin_ = new QSpinBox();
    patternBarsSpin_->setRange(kMinPatternBars, kMaxPatternBars);
    stepsPerBarSpin_ = new QSpinBox();
    stepsPerBarSpin_->setRange(kMinStepsPerBar, kMaxStepsPerBar);
    stepsPerBarSpin_->setSingleStep(4);
    editBarSpin_ = new QSpinBox();
    editBarSpin_->setRange(kMinPatternBars, kMaxPatternBars);
    repeatSpin_ = new QSpinBox();
    repeatSpin_->setRange(1, 64);
    mutationEnabledCheck_ = new QCheckBox("Enable Mutation");
    swingSlider_ = new QSlider(Qt::Horizontal);
    swingSlider_->setRange(0, 45);
    mutationSlider_ = new QSlider(Qt::Horizontal);
    mutationSlider_->setRange(0, 100);

    controlLayout->addWidget(playButton_, 0, 0);
    controlLayout->addWidget(regenerateButton, 0, 1);
    controlLayout->addWidget(mutateButton, 0, 2);
    controlLayout->addWidget(mutationEnabledCheck_, 0, 3);
    controlLayout->addWidget(new QLabel("BPM"), 1, 0);
    controlLayout->addWidget(bpmSpin_, 1, 1);
    controlLayout->addWidget(new QLabel("Pattern Bars"), 1, 2);
    controlLayout->addWidget(patternBarsSpin_, 1, 3);
    controlLayout->addWidget(new QLabel("Steps / Bar"), 2, 0);
    controlLayout->addWidget(stepsPerBarSpin_, 2, 1);
    controlLayout->addWidget(new QLabel("Edit Bar"), 2, 2);
    controlLayout->addWidget(editBarSpin_, 2, 3);
    controlLayout->addWidget(new QLabel("Repeat Before Mutate"), 3, 0);
    controlLayout->addWidget(repeatSpin_, 3, 1);
    controlLayout->addWidget(new QLabel("Swing"), 3, 2);
    controlLayout->addWidget(swingSlider_, 3, 3);
    controlLayout->addWidget(new QLabel("Mutation Amount"), 4, 0);
    controlLayout->addWidget(mutationSlider_, 4, 1, 1, 3);
    performanceLayout->addWidget(controlBox);

    auto* instrumentAddBox = new QGroupBox("Instruments");
    auto* instrumentAddLayout = new QHBoxLayout(instrumentAddBox);
    newInstrumentNameEdit_ = new QLineEdit();
    newInstrumentNameEdit_->setPlaceholderText("New instrument name");
    newInstrumentRoleCombo_ = new QComboBox();
    populateRoleCombo(newInstrumentRoleCombo_);
    addInstrumentButton_ = new QPushButton("Add Instrument");
    instrumentAddLayout->addWidget(new QLabel("Name"));
    instrumentAddLayout->addWidget(newInstrumentNameEdit_, 1);
    instrumentAddLayout->addWidget(new QLabel("Role"));
    instrumentAddLayout->addWidget(newInstrumentRoleCombo_);
    instrumentAddLayout->addWidget(addInstrumentButton_);
    instrumentsTabLayout->addWidget(instrumentAddBox);

    auto* soundfontBox = new QGroupBox("SoundFont");
    auto* soundfontLayout = new QHBoxLayout(soundfontBox);
    loadSoundfontButton_ = new QPushButton("Load SF2");
    clearSoundfontButton_ = new QPushButton("Clear SF2");
    soundfontLabel_ = new QLabel("No soundfont loaded");
    soundfontLayout->addWidget(loadSoundfontButton_);
    soundfontLayout->addWidget(clearSoundfontButton_);
    soundfontLayout->addWidget(soundfontLabel_, 1);
    soundfontTabLayout->addWidget(soundfontBox);

    auto* recordingBox = new QGroupBox("Recording + Export");
    auto* recordingLayout = new QGridLayout(recordingBox);
    recordWavButton_ = new QPushButton("Record WAV");
    recordFlacButton_ = new QPushButton("Record FLAC");
    stopRecordButton_ = new QPushButton("Stop Recording");
    exportBarsSpin_ = new QSpinBox();
    exportBarsSpin_->setRange(1, 512);
    exportSecondsSpin_ = new QDoubleSpinBox();
    exportSecondsSpin_->setRange(0.1, 3600.0);
    exportSecondsSpin_->setDecimals(1);
    exportSecondsSpin_->setSingleStep(1.0);
    renderBarsWavButton_ = new QPushButton("Render WAV By Bars");
    renderSecondsWavButton_ = new QPushButton("Render WAV By Seconds");
    recordingLayout->addWidget(recordWavButton_, 0, 0);
    recordingLayout->addWidget(recordFlacButton_, 0, 1);
    recordingLayout->addWidget(stopRecordButton_, 0, 2);
    recordingLayout->addWidget(new QLabel("Bars"), 1, 0);
    recordingLayout->addWidget(exportBarsSpin_, 1, 1);
    recordingLayout->addWidget(renderBarsWavButton_, 1, 2);
    recordingLayout->addWidget(new QLabel("Seconds"), 2, 0);
    recordingLayout->addWidget(exportSecondsSpin_, 2, 1);
    recordingLayout->addWidget(renderSecondsWavButton_, 2, 2);
    exportTabLayout->addWidget(recordingBox);

    auto* editorScroll = new QScrollArea();
    editorScroll->setWidgetResizable(true);
    instrumentEditorWidget_ = new QWidget();
    instrumentEditorLayout_ = new QVBoxLayout(instrumentEditorWidget_);
    instrumentEditorLayout_->setSpacing(12);
    editorScroll->setWidget(instrumentEditorWidget_);
    instrumentsTabLayout->addWidget(editorScroll, 1);

    auto* stepBox = new QGroupBox("Step Grid");
    auto* stepBoxLayout = new QVBoxLayout(stepBox);
    auto* stepScroll = new QScrollArea();
    stepScroll->setWidgetResizable(true);
    stepGridWidget_ = new QWidget();
    stepGridLayout_ = new QGridLayout(stepGridWidget_);
    stepGridLayout_->setSpacing(4);
    stepScroll->setWidget(stepGridWidget_);
    stepBoxLayout->addWidget(stepScroll);
    performanceLayout->addWidget(stepBox, 1);

    performanceLayout->addStretch(1);
    soundfontTabLayout->addStretch(1);
    exportTabLayout->addStretch(1);

    tabs->addTab(performanceTab, "Performance");
    tabs->addTab(instrumentsTab, "Instruments");
    tabs->addTab(soundfontTab, "SoundFont");
    tabs->addTab(exportTab, "Export");
    outer->addWidget(tabs, 1);

    connect(playButton_, &QPushButton::clicked, this, [this]() {
        controller_->setPlaying(controller_->isPlaying() == false);
        updateStatus();
    });
    connect(regenerateButton, &QPushButton::clicked, this, [this]() {
        controller_->regenerateScene();
        lastMessage_ = "Generated a fresh scene";
        refreshFromScene();
    });
    connect(mutateButton, &QPushButton::clicked, this, [this]() {
        controller_->mutateScene();
        lastMessage_ = "Mutated the current scene";
        refreshFromScene();
    });
    connect(bpmSpin_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        controller_->setBpm(value);
    });
    connect(patternBarsSpin_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        controller_->setPatternBars(value);
        if (editBarSpin_->value() > value) {
            editBarSpin_->setValue(value);
        }
        refreshFromScene();
    });
    connect(stepsPerBarSpin_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        controller_->setStepsPerBar(value);
        refreshFromScene();
    });
    connect(editBarSpin_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int) {
        refreshFromScene();
    });
    connect(repeatSpin_, qOverload<int>(&QSpinBox::valueChanged), this, [this](int value) {
        controller_->setRepeatsBeforeMutation(value);
        refreshFromScene();
    });
    connect(mutationEnabledCheck_, &QCheckBox::toggled, this, [this](bool checked) {
        controller_->setMutationEnabled(checked);
        refreshFromScene();
    });
    connect(swingSlider_, &QSlider::valueChanged, this, [this](int value) {
        controller_->setSwing(static_cast<float>(value) / 100.0f);
    });
    connect(mutationSlider_, &QSlider::valueChanged, this, [this](int value) {
        controller_->setMutationAmount(static_cast<float>(value) / 100.0f);
    });
    connect(addInstrumentButton_, &QPushButton::clicked, this, [this]() {
        const QString proposedName = newInstrumentNameEdit_->text().trimmed();
        controller_->addInstrument(proposedName.toStdString(), roleFromCombo(newInstrumentRoleCombo_));
        newInstrumentNameEdit_->clear();
        lastMessage_ = "Added instrument";
        refreshFromScene();
    });
    connect(loadSoundfontButton_, &QPushButton::clicked, this, [this]() {
        loadSoundfontFile();
    });
    connect(clearSoundfontButton_, &QPushButton::clicked, this, [this]() {
        controller_->clearSoundfont();
        lastMessage_ = "Soundfont cleared";
        refreshFromScene();
    });
    connect(recordWavButton_, &QPushButton::clicked, this, [this]() {
        startRecordingToFile(AudioFileFormat::Wav);
    });
    connect(recordFlacButton_, &QPushButton::clicked, this, [this]() {
        startRecordingToFile(AudioFileFormat::Flac);
    });
    connect(stopRecordButton_, &QPushButton::clicked, this, [this]() {
        controller_->stopRecording();
        lastMessage_ = "Recording stopped";
        refreshFromScene();
    });
    connect(renderBarsWavButton_, &QPushButton::clicked, this, [this]() {
        renderBarsToWav();
    });
    connect(renderSecondsWavButton_, &QPushButton::clicked, this, [this]() {
        renderSecondsToWav();
    });

    setCentralWidget(central);
    setStyleSheet(
        "QWidget { background: #111318; color: #f3f1e8; }"
        "QGroupBox { border: 1px solid #343846; margin-top: 12px; padding-top: 12px; font-weight: bold; }"
        "QPushButton { background: #212734; border: 1px solid #3b455b; padding: 8px; min-width: 36px; }"
        "QPushButton:hover { background: #293143; }"
        "QCheckBox { spacing: 8px; }"
        "QSlider::groove:horizontal { background: #212734; height: 8px; }"
        "QSlider::handle:horizontal { background: #e0a458; width: 16px; margin: -4px 0; }"
        "QSpinBox, QDoubleSpinBox, QLineEdit, QComboBox { background: #171b24; border: 1px solid #394154; padding: 4px; }"
        "QLabel#transportLabel { color: #e0a458; font-size: 18px; font-weight: bold; }");
}

void MainWindow::rebuildInstrumentEditors() {
    clearLayout(instrumentEditorLayout_);
    instrumentWidgets_.clear();

    const GrooveScene scene = controller_->scene();
    for (int instrumentIndex = 0; instrumentIndex < static_cast<int>(scene.instruments.size()); ++instrumentIndex) {
        auto* box = new QGroupBox(QString("Instrument %1").arg(instrumentIndex + 1));
        auto* layout = new QGridLayout(box);

        InstrumentWidgets widgets;
        widgets.nameEdit = new QLineEdit();
        widgets.roleCombo = new QComboBox();
        populateRoleCombo(widgets.roleCombo);
        widgets.densitySlider = new QSlider(Qt::Horizontal);
        widgets.densitySlider->setRange(0, 100);
        widgets.synthCheck = new QCheckBox("Synth");
        widgets.sampleCheck = new QCheckBox("Sample");
        widgets.soundfontCheck = new QCheckBox("SF2");
        widgets.midiCheck = new QCheckBox("MIDI");
        widgets.midiChannelSpin = new QSpinBox();
        widgets.midiChannelSpin->setRange(1, 16);
        widgets.soundfontChannelSpin = new QSpinBox();
        widgets.soundfontChannelSpin->setRange(1, 16);
        widgets.soundfontBankSpin = new QSpinBox();
        widgets.soundfontBankSpin->setRange(0, 16383);
        widgets.soundfontProgramSpin = new QSpinBox();
        widgets.soundfontProgramSpin->setRange(0, 127);
        widgets.soundfontPresetCombo = new QComboBox();
        for (const auto& preset : kGmPresetChoices) {
            widgets.soundfontPresetCombo->addItem(QString::fromUtf8(preset.name), QVariant(preset.bank));
        }
        widgets.loadSampleButton = new QPushButton("Load Sample");
        widgets.clearSampleButton = new QPushButton("Clear Sample");
        widgets.sampleLabel = new QLabel("No sample loaded");

        layout->addWidget(new QLabel("Name"), 0, 0);
        layout->addWidget(widgets.nameEdit, 0, 1);
        layout->addWidget(new QLabel("Role"), 0, 2);
        layout->addWidget(widgets.roleCombo, 0, 3);
        layout->addWidget(new QLabel("Density"), 1, 0);
        layout->addWidget(widgets.densitySlider, 1, 1, 1, 3);
        layout->addWidget(widgets.synthCheck, 2, 0);
        layout->addWidget(widgets.sampleCheck, 2, 1);
        layout->addWidget(widgets.soundfontCheck, 2, 2);
        layout->addWidget(widgets.midiCheck, 2, 3);
        layout->addWidget(new QLabel("MIDI Ch"), 3, 0);
        layout->addWidget(widgets.midiChannelSpin, 3, 1);
        layout->addWidget(new QLabel("SF2 Ch"), 3, 2);
        layout->addWidget(widgets.soundfontChannelSpin, 3, 3);
        layout->addWidget(new QLabel("SF2 Bank"), 4, 0);
        layout->addWidget(widgets.soundfontBankSpin, 4, 1);
        layout->addWidget(new QLabel("SF2 Program"), 4, 2);
        layout->addWidget(widgets.soundfontProgramSpin, 4, 3);
        layout->addWidget(new QLabel("GM Preset"), 5, 0);
        layout->addWidget(widgets.soundfontPresetCombo, 5, 1, 1, 3);
        layout->addWidget(widgets.loadSampleButton, 6, 0);
        layout->addWidget(widgets.clearSampleButton, 6, 1);
        layout->addWidget(widgets.sampleLabel, 6, 2, 1, 2);

        connect(widgets.nameEdit, &QLineEdit::editingFinished, this, [this, instrumentIndex, edit = widgets.nameEdit]() {
            controller_->setInstrumentName(instrumentIndex, edit->text().trimmed().toStdString());
            refreshFromScene();
        });
        connect(widgets.roleCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, instrumentIndex, combo = widgets.roleCombo](int) {
            controller_->setInstrumentRole(instrumentIndex, roleFromCombo(combo));
            refreshFromScene();
        });
        connect(widgets.densitySlider, &QSlider::valueChanged, this, [this, instrumentIndex](int value) {
            controller_->setInstrumentDensity(instrumentIndex, static_cast<float>(value) / 100.0f);
        });
        connect(widgets.synthCheck, &QCheckBox::toggled, this, [this, instrumentIndex](bool checked) {
            controller_->setInstrumentSynthEnabled(instrumentIndex, checked);
            refreshFromScene();
        });
        connect(widgets.sampleCheck, &QCheckBox::toggled, this, [this, instrumentIndex](bool checked) {
            controller_->setInstrumentSampleEnabled(instrumentIndex, checked);
            refreshFromScene();
        });
        connect(widgets.soundfontCheck, &QCheckBox::toggled, this, [this, instrumentIndex](bool checked) {
            controller_->setInstrumentSoundfontEnabled(instrumentIndex, checked);
            refreshFromScene();
        });
        connect(widgets.midiCheck, &QCheckBox::toggled, this, [this, instrumentIndex](bool checked) {
            controller_->setInstrumentMidiEnabled(instrumentIndex, checked);
            refreshFromScene();
        });
        connect(widgets.midiChannelSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this, instrumentIndex](int value) {
            controller_->setInstrumentMidiChannel(instrumentIndex, value);
        });
        connect(widgets.soundfontChannelSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this, instrumentIndex](int value) {
            controller_->setInstrumentSoundfontChannel(instrumentIndex, value);
        });
        connect(widgets.soundfontBankSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this, instrumentIndex](int value) {
            controller_->setInstrumentSoundfontBank(instrumentIndex, value);
        });
        connect(widgets.soundfontProgramSpin, qOverload<int>(&QSpinBox::valueChanged), this, [this, instrumentIndex](int value) {
            controller_->setInstrumentSoundfontProgram(instrumentIndex, value);
        });
        connect(widgets.soundfontPresetCombo, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this, instrumentIndex, combo = widgets.soundfontPresetCombo](int index) {
                if (index <= 0) {
                    refreshFromScene();
                    return;
                }

                const auto& preset = kGmPresetChoices[index];
                controller_->setInstrumentSoundfontBank(instrumentIndex, preset.bank);
                controller_->setInstrumentSoundfontProgram(instrumentIndex, preset.program);
                if (preset.bank == 128) {
                    controller_->setInstrumentSoundfontChannel(instrumentIndex, 10);
                }
                refreshFromScene();
            });
        connect(widgets.loadSampleButton, &QPushButton::clicked, this, [this, instrumentIndex]() {
            loadSampleForInstrument(instrumentIndex);
        });
        connect(widgets.clearSampleButton, &QPushButton::clicked, this, [this, instrumentIndex]() {
            controller_->clearSample(instrumentIndex);
            lastMessage_ = QString("Cleared sample for instrument %1").arg(instrumentIndex + 1);
            refreshFromScene();
        });

        instrumentEditorLayout_->addWidget(box);
        instrumentWidgets_.push_back(widgets);
    }
    instrumentEditorLayout_->addStretch(1);
}

void MainWindow::rebuildStepGrid() {
    clearLayout(stepGridLayout_);
    stepButtons_.clear();
    stepRowLabels_.clear();

    const GrooveScene scene = controller_->scene();
    for (int stepIndex = 0; stepIndex < scene.stepsPerBar; ++stepIndex) {
        auto* marker = new QLabel(QString::number(stepIndex + 1));
        marker->setAlignment(Qt::AlignCenter);
        stepGridLayout_->addWidget(marker, 0, stepIndex + 1);
    }

    for (int instrumentIndex = 0; instrumentIndex < static_cast<int>(scene.instruments.size()); ++instrumentIndex) {
        auto* label = new QLabel(QString::fromStdString(scene.instruments[static_cast<std::size_t>(instrumentIndex)].name));
        stepGridLayout_->addWidget(label, instrumentIndex + 1, 0);
        stepRowLabels_.push_back(label);

        std::vector<QPushButton*> rowButtons;
        for (int stepIndex = 0; stepIndex < scene.stepsPerBar; ++stepIndex) {
            auto* button = makeStepButton(instrumentIndex, stepIndex);
            stepGridLayout_->addWidget(button, instrumentIndex + 1, stepIndex + 1);
            rowButtons.push_back(button);
        }
        stepButtons_.push_back(rowButtons);
    }
}

void MainWindow::clearLayout(QLayout* layout) {
    if (layout == nullptr) {
        return;
    }
    while (auto* item = layout->takeAt(0)) {
        if (item->widget() != nullptr) {
            item->widget()->deleteLater();
        }
        if (item->layout() != nullptr) {
            clearLayout(item->layout());
            item->layout()->deleteLater();
        }
        delete item;
    }
}

void MainWindow::populateRoleCombo(QComboBox* combo) const {
    combo->clear();
    combo->addItem("Kick", static_cast<int>(InstrumentRole::Kick));
    combo->addItem("Snare", static_cast<int>(InstrumentRole::Snare));
    combo->addItem("Closed Hat", static_cast<int>(InstrumentRole::ClosedHat));
    combo->addItem("Open Hat", static_cast<int>(InstrumentRole::OpenHat));
    combo->addItem("Clap", static_cast<int>(InstrumentRole::Clap));
    combo->addItem("Perc", static_cast<int>(InstrumentRole::Perc));
    combo->addItem("Bass", static_cast<int>(InstrumentRole::Bass));
    combo->addItem("Lead", static_cast<int>(InstrumentRole::Lead));
    combo->addItem("Custom", static_cast<int>(InstrumentRole::Custom));
}

InstrumentRole MainWindow::roleFromCombo(const QComboBox* combo) const {
    return static_cast<InstrumentRole>(combo->currentData().toInt());
}

int MainWindow::roleComboIndex(InstrumentRole role) const {
    switch (role) {
    case InstrumentRole::Kick:
        return 0;
    case InstrumentRole::Snare:
        return 1;
    case InstrumentRole::ClosedHat:
        return 2;
    case InstrumentRole::OpenHat:
        return 3;
    case InstrumentRole::Clap:
        return 4;
    case InstrumentRole::Perc:
        return 5;
    case InstrumentRole::Bass:
        return 6;
    case InstrumentRole::Lead:
        return 7;
    case InstrumentRole::Custom:
        return 8;
    }
    return 8;
}

void MainWindow::refreshFromScene() {
    const GrooveScene scene = controller_->scene();
    if (instrumentWidgets_.size() != scene.instruments.size()) {
        rebuildInstrumentEditors();
    }
    if ((stepButtons_.size() != scene.instruments.size()) || ((stepButtons_.empty() == false) && (stepButtons_.front().size() != static_cast<std::size_t>(scene.stepsPerBar)))) {
        rebuildStepGrid();
    }

    bpmSpin_->blockSignals(true);
    patternBarsSpin_->blockSignals(true);
    stepsPerBarSpin_->blockSignals(true);
    editBarSpin_->blockSignals(true);
    repeatSpin_->blockSignals(true);
    mutationEnabledCheck_->blockSignals(true);
    swingSlider_->blockSignals(true);
    mutationSlider_->blockSignals(true);

    bpmSpin_->setValue(scene.bpm);
    patternBarsSpin_->setValue(scene.patternBars);
    stepsPerBarSpin_->setValue(scene.stepsPerBar);
    editBarSpin_->setMaximum(scene.patternBars);
    if (editBarSpin_->value() > scene.patternBars) {
        editBarSpin_->setValue(scene.patternBars);
    }
    if (editBarSpin_->value() < 1) {
        editBarSpin_->setValue(1);
    }
    repeatSpin_->setValue(scene.repeatsBeforeMutation);
    mutationEnabledCheck_->setChecked(scene.mutationEnabled);
    swingSlider_->setValue(static_cast<int>(scene.swing * 100.0f));
    mutationSlider_->setValue(static_cast<int>(scene.mutationAmount * 100.0f));
    soundfontLabel_->setText(soundfontLabelText(scene.soundfontPath));

    bpmSpin_->blockSignals(false);
    patternBarsSpin_->blockSignals(false);
    stepsPerBarSpin_->blockSignals(false);
    editBarSpin_->blockSignals(false);
    repeatSpin_->blockSignals(false);
    mutationEnabledCheck_->blockSignals(false);
    swingSlider_->blockSignals(false);
    mutationSlider_->blockSignals(false);

    const int barOffset = currentEditBarIndex() * scene.stepsPerBar;
    for (int instrumentIndex = 0; instrumentIndex < static_cast<int>(scene.instruments.size()); ++instrumentIndex) {
        const auto& instrument = scene.instruments[static_cast<std::size_t>(instrumentIndex)];
        auto& widgets = instrumentWidgets_[static_cast<std::size_t>(instrumentIndex)];

        if ((widgets.nameEdit->hasFocus() == false) && (widgets.nameEdit->text() != QString::fromStdString(instrument.name))) {
            widgets.nameEdit->setText(QString::fromStdString(instrument.name));
        }

        widgets.roleCombo->blockSignals(true);
        widgets.roleCombo->setCurrentIndex(roleComboIndex(instrument.role));
        widgets.roleCombo->blockSignals(false);

        widgets.densitySlider->blockSignals(true);
        widgets.densitySlider->setValue(static_cast<int>(instrument.density * 100.0f));
        widgets.densitySlider->blockSignals(false);

        widgets.synthCheck->blockSignals(true);
        widgets.sampleCheck->blockSignals(true);
        widgets.soundfontCheck->blockSignals(true);
        widgets.midiCheck->blockSignals(true);
        widgets.midiChannelSpin->blockSignals(true);
        widgets.soundfontChannelSpin->blockSignals(true);
        widgets.soundfontBankSpin->blockSignals(true);
        widgets.soundfontProgramSpin->blockSignals(true);
        widgets.soundfontPresetCombo->blockSignals(true);

        widgets.synthCheck->setChecked(instrument.layers.synthEnabled);
        widgets.sampleCheck->setChecked(instrument.layers.sampleEnabled);
        widgets.soundfontCheck->setChecked(instrument.layers.soundfontEnabled);
        widgets.midiCheck->setChecked(instrument.layers.midiEnabled);
        widgets.midiChannelSpin->setValue(instrument.layers.midiChannel);
        widgets.soundfontChannelSpin->setValue(instrument.layers.soundfontChannel);
        widgets.soundfontBankSpin->setValue(instrument.layers.soundfontBank);
        widgets.soundfontProgramSpin->setValue(instrument.layers.soundfontProgram);
        widgets.soundfontPresetCombo->setCurrentIndex(gmPresetIndexFor(instrument.layers.soundfontBank, instrument.layers.soundfontProgram));

        widgets.synthCheck->blockSignals(false);
        widgets.sampleCheck->blockSignals(false);
        widgets.soundfontCheck->blockSignals(false);
        widgets.midiCheck->blockSignals(false);
        widgets.midiChannelSpin->blockSignals(false);
        widgets.soundfontChannelSpin->blockSignals(false);
        widgets.soundfontBankSpin->blockSignals(false);
        widgets.soundfontProgramSpin->blockSignals(false);
        widgets.soundfontPresetCombo->blockSignals(false);

        const bool hasSample = instrument.layers.samplePath.empty() == false;
        const bool hasSoundfont = scene.soundfontPath.empty() == false;
        widgets.sampleCheck->setEnabled(hasSample);
        widgets.clearSampleButton->setEnabled(hasSample);
        widgets.soundfontCheck->setEnabled(hasSoundfont);
        widgets.soundfontPresetCombo->setEnabled(hasSoundfont);
        widgets.sampleLabel->setText(sampleLabelText(instrument.layers.samplePath));

        if (instrumentIndex < static_cast<int>(stepRowLabels_.size())) {
            stepRowLabels_[static_cast<std::size_t>(instrumentIndex)]->setText(QString::fromStdString(instrument.name));
        }

        for (int stepIndex = 0; stepIndex < scene.stepsPerBar; ++stepIndex) {
            const int absoluteStep = barOffset + stepIndex;
            const bool active = absoluteStep < totalStepCount(scene) && instrument.steps[static_cast<std::size_t>(absoluteStep)].active;
            stepButtons_[static_cast<std::size_t>(instrumentIndex)][static_cast<std::size_t>(stepIndex)]->setText(active ? "X" : ".");
        }
    }

    refreshStepHighlight();
    updateStatus();
}

void MainWindow::refreshStepHighlight() {
    const GrooveScene scene = controller_->scene();
    const int currentStep = controller_->currentStep();
    const int barOffset = currentEditBarIndex() * scene.stepsPerBar;
    for (int instrumentIndex = 0; instrumentIndex < static_cast<int>(stepButtons_.size()); ++instrumentIndex) {
        for (int stepIndex = 0; stepIndex < static_cast<int>(stepButtons_[static_cast<std::size_t>(instrumentIndex)].size()); ++stepIndex) {
            const int absoluteStep = barOffset + stepIndex;
            const bool active = absoluteStep < totalStepCount(scene) && scene.instruments[static_cast<std::size_t>(instrumentIndex)].steps[static_cast<std::size_t>(absoluteStep)].active;
            const bool highlighted = absoluteStep == currentStep;
            stepButtons_[static_cast<std::size_t>(instrumentIndex)][static_cast<std::size_t>(stepIndex)]->setStyleSheet(stepButtonStyle(active, highlighted));
        }
    }
}

void MainWindow::updateStatus() {
    const GrooveScene scene = controller_->scene();
    playButton_->setText(controller_->isPlaying() ? "Stop" : "Play");
    transportLabel_->setText(controller_->isPlaying() ? "RUN" : "STOP");
    stopRecordButton_->setEnabled(controller_->isRecording());
    recordWavButton_->setEnabled(controller_->isRecording() == false);
    recordFlacButton_->setEnabled(controller_->isRecording() == false);

    QStringList parts;
    parts << QString("Tempo %1 BPM").arg(scene.bpm)
          << QString("Pattern %1 bar(s)").arg(scene.patternBars)
          << QString("Grid %1 step(s)/bar").arg(scene.stepsPerBar)
          << QString("Edit bar %1").arg(editBarSpin_->value())
          << QString("Instruments %1").arg(scene.instruments.size())
          << QString("Mutate every %1 loop(s)").arg(scene.repeatsBeforeMutation)
          << QString("Swing %1%").arg(static_cast<int>(scene.swing * 100.0f))
          << QString("Mutation %1%").arg(static_cast<int>(scene.mutationAmount * 100.0f))
          << (scene.mutationEnabled ? "Mutation on" : "Mutation off")
          << soundfontLabelText(scene.soundfontPath);

    if (controller_->isRecording()) {
        parts << QString("REC %1").arg(sampleLabelText(controller_->recordingPath()));
    }
    if (controller_->audioReady() == false) {
        parts << "JACK unavailable: UI is running but audio is offline";
    }
    if (lastMessage_.isEmpty() == false) {
        parts << lastMessage_;
    }
    statusLabel_->setText(parts.join(" | "));
}

void MainWindow::loadSampleForInstrument(int instrumentIndex) {
    const QString filePath = QFileDialog::getOpenFileName(this, QString("Load Sample for Instrument %1").arg(instrumentIndex + 1), QString(), "Audio Files (*.wav *.flac)");
    if (filePath.isEmpty()) {
        return;
    }

    if (controller_->loadSample(instrumentIndex, filePath.toStdString()) == false) {
        QMessageBox::warning(this, "Sample Load Failed", "Could not open that WAV or FLAC file.");
        return;
    }

    lastMessage_ = QString("Loaded sample %1").arg(QFileInfo(filePath).fileName());
    refreshFromScene();
}

void MainWindow::loadSoundfontFile() {
    const QString filePath = QFileDialog::getOpenFileName(this, "Load SoundFont", QString(), "SoundFont Files (*.sf2)");
    if (filePath.isEmpty()) {
        return;
    }

    if (controller_->loadSoundfont(filePath.toStdString()) == false) {
        QMessageBox::warning(this, "SoundFont Load Failed", "Could not open that SoundFont file.");
        return;
    }

    lastMessage_ = QString("Loaded soundfont %1").arg(QFileInfo(filePath).fileName());
    refreshFromScene();
}

void MainWindow::renderBarsToWav() {
    const QString defaultName = QString("%1/groove-forge-render-%2.wav")
        .arg(QDir::homePath())
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss"));
    const QString path = QFileDialog::getSaveFileName(this, "Render WAV By Bars", defaultName, "WAV Files (*.wav)");
    if (path.isEmpty()) {
        return;
    }

    if (controller_->exportWavBars(path.toStdString(), exportBarsSpin_->value()) == false) {
        QMessageBox::warning(this, "Render Failed", "Could not render the current scene to WAV.");
        return;
    }

    lastMessage_ = QString("Rendered %1 bar(s) to %2").arg(exportBarsSpin_->value()).arg(QFileInfo(path).fileName());
    refreshFromScene();
}

void MainWindow::renderSecondsToWav() {
    const QString defaultName = QString("%1/groove-forge-render-%2.wav")
        .arg(QDir::homePath())
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss"));
    const QString path = QFileDialog::getSaveFileName(this, "Render WAV By Seconds", defaultName, "WAV Files (*.wav)");
    if (path.isEmpty()) {
        return;
    }

    if (controller_->exportWavSeconds(path.toStdString(), exportSecondsSpin_->value()) == false) {
        QMessageBox::warning(this, "Render Failed", "Could not render the current scene to WAV.");
        return;
    }

    lastMessage_ = QString("Rendered %1 second(s) to %2").arg(exportSecondsSpin_->value()).arg(QFileInfo(path).fileName());
    refreshFromScene();
}

void MainWindow::startRecordingToFile(AudioFileFormat format) {
    const QString extension = format == AudioFileFormat::Wav ? "wav" : "flac";
    const QString defaultName = QString("%1/groove-forge-%2.%3")
        .arg(QDir::homePath())
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss"))
        .arg(extension);
    const QString filter = format == AudioFileFormat::Wav ? "WAV Files (*.wav)" : "FLAC Files (*.flac)";
    const QString path = QFileDialog::getSaveFileName(this, QString("Record %1").arg(audioFileFormatName(format)), defaultName, filter);
    if (path.isEmpty()) {
        return;
    }

    if (controller_->startRecording(path.toStdString(), format) == false) {
        QMessageBox::warning(this, "Recording Failed", "Recording needs a running JACK engine and a writable destination file.");
        return;
    }

    lastMessage_ = QString("Recording %1 to %2").arg(audioFileFormatName(format), QFileInfo(path).fileName());
    refreshFromScene();
}

int MainWindow::currentEditBarIndex() const {
    return std::max(0, editBarSpin_->value() - 1);
}

QPushButton* MainWindow::makeStepButton(int instrumentIndex, int stepIndex) {
    auto* button = new QPushButton(".");
    connect(button, &QPushButton::clicked, this, [this, instrumentIndex, stepIndex]() {
        const GrooveScene scene = controller_->scene();
        const int absoluteStep = currentEditBarIndex() * scene.stepsPerBar + stepIndex;
        controller_->toggleStep(instrumentIndex, absoluteStep);
        refreshFromScene();
    });
    return button;
}

QString MainWindow::stepButtonStyle(bool active, bool currentStep) const {
    const QString background = active ? "#e0a458" : "#1b2029";
    const QString foreground = active ? "#111318" : "#95a0b6";
    const QString border = currentStep ? "#f25f5c" : "#3a455b";
    return QString("QPushButton { background: %1; color: %2; border: 2px solid %3; font-weight: bold; }")
        .arg(background, foreground, border);
}

QString MainWindow::sampleLabelText(const std::string& path) const {
    if (path.empty()) {
        return "No sample loaded";
    }
    return QFileInfo(QString::fromStdString(path)).fileName();
}

QString MainWindow::soundfontLabelText(const std::string& path) const {
    if (path.empty()) {
        return "No soundfont loaded";
    }
    return QString("SF2 %1").arg(QFileInfo(QString::fromStdString(path)).fileName());
}

}  // namespace groove
