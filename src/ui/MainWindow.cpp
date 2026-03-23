#include "ui/MainWindow.h"

#include <algorithm>

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QMediaPlayer>
#include <QMediaContent>
#include <QPushButton>
#include <QSizePolicy>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

namespace groove {

namespace {

QJsonObject stepToJson(const Step& step) {
    QJsonObject object;
    object["active"] = step.active;
    object["velocity"] = step.velocity;
    object["note"] = step.note;
    object["attack"] = step.attack;
    object["decay"] = step.decay;
    object["sustain"] = step.sustain;
    object["release"] = step.release;
    object["gate"] = step.gate;
    return object;
}

Step stepFromJson(const QJsonObject& object) {
    Step step;
    step.active = object["active"].toBool(false);
    step.velocity = static_cast<float>(object["velocity"].toDouble(0.0));
    step.note = object["note"].toInt(60);
    step.attack = static_cast<float>(object["attack"].toDouble(step.attack));
    step.decay = static_cast<float>(object["decay"].toDouble(step.decay));
    step.sustain = static_cast<float>(object["sustain"].toDouble(step.sustain));
    step.release = static_cast<float>(object["release"].toDouble(step.release));
    step.gate = static_cast<float>(object["gate"].toDouble(step.gate));
    return step;
}

QJsonObject instrumentToJson(const InstrumentDefinition& instrument) {
    QJsonObject object;
    object["name"] = QString::fromStdString(instrument.name);
    object["role"] = static_cast<int>(instrument.role);
    object["density"] = instrument.density;
    object["rootNote"] = instrument.rootNote;

    QJsonObject layers;
    layers["sampleEnabled"] = instrument.layers.sampleEnabled;
    layers["midiEnabled"] = instrument.layers.midiEnabled;
    layers["soundfontEnabled"] = instrument.layers.soundfontEnabled;
    layers["midiChannel"] = instrument.layers.midiChannel;
    layers["sampleRootMidiNote"] = instrument.layers.sampleRootMidiNote;
    layers["soundfontChannel"] = instrument.layers.soundfontChannel;
    layers["soundfontBank"] = instrument.layers.soundfontBank;
    layers["soundfontProgram"] = instrument.layers.soundfontProgram;
    layers["samplePath"] = QString::fromStdString(instrument.layers.samplePath);
    object["layers"] = layers;

    QJsonArray steps;
    for (const auto& step : instrument.steps) {
        steps.append(stepToJson(step));
    }
    object["steps"] = steps;
    return object;
}

InstrumentDefinition instrumentFromJson(const QJsonObject& object) {
    InstrumentDefinition instrument = makeInstrument(
        static_cast<InstrumentRole>(object["role"].toInt(static_cast<int>(InstrumentRole::Custom))),
        object["name"].toString().trimmed().toStdString());
    instrument.density = static_cast<float>(object["density"].toDouble(instrument.density));
    instrument.rootNote = object["rootNote"].toInt(instrument.rootNote);

    const QJsonObject layers = object["layers"].toObject();
    instrument.layers.sampleEnabled = layers["sampleEnabled"].toBool(instrument.layers.sampleEnabled);
    instrument.layers.midiEnabled = layers["midiEnabled"].toBool(instrument.layers.midiEnabled);
    instrument.layers.soundfontEnabled = layers["soundfontEnabled"].toBool(instrument.layers.soundfontEnabled);
    instrument.layers.midiChannel = layers["midiChannel"].toInt(instrument.layers.midiChannel);
    instrument.layers.sampleRootMidiNote = layers["sampleRootMidiNote"].toInt(instrument.layers.sampleRootMidiNote);
    instrument.layers.soundfontChannel = layers["soundfontChannel"].toInt(instrument.layers.soundfontChannel);
    instrument.layers.soundfontBank = layers["soundfontBank"].toInt(instrument.layers.soundfontBank);
    instrument.layers.soundfontProgram = layers["soundfontProgram"].toInt(instrument.layers.soundfontProgram);
    instrument.layers.samplePath = layers["samplePath"].toString().toStdString();

    instrument.steps.clear();
    for (const auto& stepValue : object["steps"].toArray()) {
        instrument.steps.push_back(stepFromJson(stepValue.toObject()));
    }
    return instrument;
}

QJsonObject sceneToJson(const GrooveScene& scene) {
    QJsonObject object;
    object["bpm"] = scene.bpm;
    object["patternBars"] = scene.patternBars;
    object["stepsPerBar"] = scene.stepsPerBar;
    object["repeatsBeforeMutation"] = scene.repeatsBeforeMutation;
    object["swing"] = scene.swing;
    object["mutationAmount"] = scene.mutationAmount;
    object["mutationEnabled"] = scene.mutationEnabled;
    object["soundfontPath"] = QString::fromStdString(scene.soundfontPath);
    object["seed"] = static_cast<int>(scene.seed);

    QJsonArray instruments;
    for (const auto& instrument : scene.instruments) {
        instruments.append(instrumentToJson(instrument));
    }
    object["instruments"] = instruments;
    return object;
}

GrooveScene sceneFromJson(const QJsonObject& object) {
    GrooveScene scene;
    scene.bpm = object["bpm"].toInt(scene.bpm);
    scene.patternBars = object["patternBars"].toInt(scene.patternBars);
    scene.stepsPerBar = object["stepsPerBar"].toInt(scene.stepsPerBar);
    scene.repeatsBeforeMutation = object["repeatsBeforeMutation"].toInt(scene.repeatsBeforeMutation);
    scene.swing = static_cast<float>(object["swing"].toDouble(scene.swing));
    scene.mutationAmount = static_cast<float>(object["mutationAmount"].toDouble(scene.mutationAmount));
    scene.mutationEnabled = object["mutationEnabled"].toBool(scene.mutationEnabled);
    scene.soundfontPath = object["soundfontPath"].toString().toStdString();
    scene.seed = static_cast<std::uint32_t>(object["seed"].toInt(static_cast<int>(scene.seed)));

    scene.instruments.clear();
    for (const auto& instrumentValue : object["instruments"].toArray()) {
        scene.instruments.push_back(instrumentFromJson(instrumentValue.toObject()));
    }
    return scene;
}

int comboIndexForPreset(const QComboBox* combo, int bank, int program) {
    for (int index = 0; index < combo->count(); ++index) {
        if ((combo->itemData(index, Qt::UserRole).toInt() == bank)
            && (combo->itemData(index, Qt::UserRole + 1).toInt() == program)) {
            return index;
        }
    }
    return 0;
}

void populatePresetCombo(QComboBox* combo, const std::vector<SoundFontPreset>& presets) {
    combo->clear();
    combo->addItem("Custom Bank/Program");
    combo->setItemData(0, -1, Qt::UserRole);
    combo->setItemData(0, -1, Qt::UserRole + 1);

    for (const auto& preset : presets) {
        QString label = QString("[%1:%2] %3").arg(preset.bank).arg(preset.program).arg(QString::fromStdString(preset.name));
        combo->addItem(label);
        const int index = combo->count() - 1;
        combo->setItemData(index, preset.bank, Qt::UserRole);
        combo->setItemData(index, preset.program, Qt::UserRole + 1);
    }
}

}  // namespace

MainWindow::MainWindow(GrooveController* controller, QWidget* parent)
    : QMainWindow(parent), controller_(controller) {
    previewPlayer_ = new QMediaPlayer(this);
    buildUi();
    refreshFromScene();

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, [this]() {
        controller_->tickAutomation();
        syncActiveBarToTransport();
        refreshFromScene();
    });
    timer_->start(50);
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    updateStepGridButtonSizing();
    if (stepGridWidget_ != nullptr) {
        stepGridWidget_->adjustSize();
    }
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

    auto* projectBox = new QGroupBox("Project");
    auto* projectLayout = new QHBoxLayout(projectBox);
    auto* saveProjectButton = new QPushButton("Save Project");
    auto* loadProjectButton = new QPushButton("Load Project");
    projectLayout->addWidget(saveProjectButton);
    projectLayout->addWidget(loadProjectButton);
    projectLayout->addStretch(1);
    instrumentsTabLayout->addWidget(projectBox);

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
    previewFileLabel_ = new QLabel("No preview file loaded");
    previewPlayButton_ = new QPushButton("Play");
    previewStopButton_ = new QPushButton("Stop");
    previewRewindButton_ = new QPushButton("RW");
    previewForwardButton_ = new QPushButton("FF");
    recordingLayout->addWidget(recordWavButton_, 0, 0);
    recordingLayout->addWidget(recordFlacButton_, 0, 1);
    recordingLayout->addWidget(stopRecordButton_, 0, 2);
    recordingLayout->addWidget(new QLabel("Bars"), 1, 0);
    recordingLayout->addWidget(exportBarsSpin_, 1, 1);
    recordingLayout->addWidget(renderBarsWavButton_, 1, 2);
    recordingLayout->addWidget(new QLabel("Seconds"), 2, 0);
    recordingLayout->addWidget(exportSecondsSpin_, 2, 1);
    recordingLayout->addWidget(renderSecondsWavButton_, 2, 2);
    recordingLayout->addWidget(new QLabel("Preview File"), 3, 0);
    recordingLayout->addWidget(previewFileLabel_, 3, 1, 1, 2);
    recordingLayout->addWidget(previewRewindButton_, 4, 0);
    recordingLayout->addWidget(previewPlayButton_, 4, 1);
    recordingLayout->addWidget(previewStopButton_, 4, 2);
    recordingLayout->addWidget(previewForwardButton_, 4, 3);
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
    stepScrollArea_ = new QScrollArea();
    stepScrollArea_->setWidgetResizable(false);
    stepScrollArea_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    stepScrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    stepScrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    stepScrollArea_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    stepGridWidget_ = new QWidget();
    stepGridLayout_ = new QGridLayout(stepGridWidget_);
    stepGridLayout_->setSpacing(4);
    stepGridLayout_->setContentsMargins(6, 6, 6, 6);
    stepScrollArea_->setWidget(stepGridWidget_);
    stepBoxLayout->addWidget(stepScrollArea_);
    stepBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    performanceLayout->addWidget(stepBox, 1);

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
    connect(saveProjectButton, &QPushButton::clicked, this, [this]() {
        saveProject();
    });
    connect(loadProjectButton, &QPushButton::clicked, this, [this]() {
        loadProject();
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
        const QString recordedPath = QString::fromStdString(controller_->recordingPath());
        controller_->stopRecording();
        if (recordedPath.isEmpty() == false) {
            setPreviewFile(recordedPath);
        }
        lastMessage_ = "Recording stopped";
        refreshFromScene();
    });
    connect(renderBarsWavButton_, &QPushButton::clicked, this, [this]() {
        renderBarsToWav();
    });
    connect(renderSecondsWavButton_, &QPushButton::clicked, this, [this]() {
        renderSecondsToWav();
    });
    connect(previewPlayButton_, &QPushButton::clicked, this, [this]() {
        if (previewPath_.isEmpty()) {
            return;
        }
        previewPlayer_->play();
    });
    connect(previewStopButton_, &QPushButton::clicked, this, [this]() {
        previewPlayer_->stop();
    });
    connect(previewRewindButton_, &QPushButton::clicked, this, [this]() {
        seekPreview(-5000);
    });
    connect(previewForwardButton_, &QPushButton::clicked, this, [this]() {
        seekPreview(5000);
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
    const std::vector<SoundFontPreset> presets = controller_->soundfontPresets();
    for (int instrumentIndex = 0; instrumentIndex < static_cast<int>(scene.instruments.size()); ++instrumentIndex) {
        auto* box = new QGroupBox(QString("Instrument %1").arg(instrumentIndex + 1));
        auto* layout = new QGridLayout(box);
        auto* moveUpButton = new QPushButton("Up");
        auto* moveDownButton = new QPushButton("Down");
        auto* removeButton = new QPushButton("Remove");

        InstrumentWidgets widgets;
        widgets.nameEdit = new QLineEdit();
        widgets.roleCombo = new QComboBox();
        populateRoleCombo(widgets.roleCombo);
        widgets.densitySlider = new QSlider(Qt::Horizontal);
        widgets.densitySlider->setRange(0, 100);
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
        populatePresetCombo(widgets.soundfontPresetCombo, presets);
        widgets.loadSampleButton = new QPushButton("Load Sample");
        widgets.clearSampleButton = new QPushButton("Clear Sample");
        widgets.sampleLabel = new QLabel("No sample loaded");

        layout->addWidget(new QLabel("Name"), 0, 0);
        layout->addWidget(widgets.nameEdit, 0, 1);
        layout->addWidget(new QLabel("Role"), 0, 2);
        layout->addWidget(widgets.roleCombo, 0, 3);
        layout->addWidget(moveUpButton, 0, 4);
        layout->addWidget(moveDownButton, 0, 5);
        layout->addWidget(removeButton, 0, 6);
        layout->addWidget(new QLabel("Density"), 1, 0);
        layout->addWidget(widgets.densitySlider, 1, 1, 1, 6);
        layout->addWidget(widgets.sampleCheck, 2, 0);
        layout->addWidget(widgets.soundfontCheck, 2, 1);
        layout->addWidget(widgets.midiCheck, 2, 2);
        layout->addWidget(new QLabel("MIDI Ch"), 3, 0);
        layout->addWidget(widgets.midiChannelSpin, 3, 1);
        layout->addWidget(new QLabel("SF2 Ch"), 3, 2);
        layout->addWidget(widgets.soundfontChannelSpin, 3, 3);
        layout->addWidget(new QLabel("SF2 Bank"), 4, 0);
        layout->addWidget(widgets.soundfontBankSpin, 4, 1);
        layout->addWidget(new QLabel("SF2 Program"), 4, 2);
        layout->addWidget(widgets.soundfontProgramSpin, 4, 3);
        layout->addWidget(new QLabel("Preset"), 5, 0);
        layout->addWidget(widgets.soundfontPresetCombo, 5, 1, 1, 6);
        layout->addWidget(widgets.loadSampleButton, 6, 0);
        layout->addWidget(widgets.clearSampleButton, 6, 1);
        layout->addWidget(widgets.sampleLabel, 6, 2, 1, 5);

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

                const int bank = combo->itemData(index, Qt::UserRole).toInt();
                const int program = combo->itemData(index, Qt::UserRole + 1).toInt();
                controller_->setInstrumentSoundfontBank(instrumentIndex, bank);
                controller_->setInstrumentSoundfontProgram(instrumentIndex, program);
                if (bank == 128) {
                    controller_->setInstrumentSoundfontChannel(instrumentIndex, 10);
                }
                refreshFromScene();
            });
        connect(moveUpButton, &QPushButton::clicked, this, [this, instrumentIndex]() {
            controller_->moveInstrumentUp(instrumentIndex);
            refreshFromScene();
        });
        connect(moveDownButton, &QPushButton::clicked, this, [this, instrumentIndex]() {
            controller_->moveInstrumentDown(instrumentIndex);
            refreshFromScene();
        });
        connect(removeButton, &QPushButton::clicked, this, [this, instrumentIndex]() {
            controller_->removeInstrument(instrumentIndex);
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

    updateStepGridButtonSizing();
    stepGridWidget_->adjustSize();
    stepGridWidget_->setMinimumSize(stepGridLayout_->sizeHint());
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

void MainWindow::updateStepGridButtonSizing() {
    if ((stepScrollArea_ == nullptr) || stepButtons_.empty()) {
        return;
    }

    const int rowCount = static_cast<int>(stepButtons_.size()) + 1;
    const int columnCount = static_cast<int>(stepButtons_.front().size()) + 1;
    const QSize viewportSize = stepScrollArea_->viewport()->size();
    if ((viewportSize.width() <= 0) || (viewportSize.height() <= 0)) {
        return;
    }

    const int usableWidth = std::max(120, viewportSize.width() - 32);
    const int usableHeight = std::max(120, viewportSize.height() - 32);
    const int buttonSize = std::clamp(std::min(usableWidth / std::max(1, columnCount), usableHeight / std::max(1, rowCount)), 26, 64);
    const int labelWidth = std::clamp(buttonSize * 3, 88, 180);

    for (auto* label : stepRowLabels_) {
        label->setMinimumWidth(labelWidth);
    }
    for (auto& row : stepButtons_) {
        for (auto* button : row) {
            button->setFixedSize(buttonSize, buttonSize);
        }
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
    const std::vector<SoundFontPreset> presets = controller_->soundfontPresets();
    if (instrumentWidgets_.size() != scene.instruments.size()) {
        rebuildInstrumentEditors();
    }
    if ((stepButtons_.size() != scene.instruments.size())
        || ((stepButtons_.empty() == false) && (stepButtons_.front().size() != static_cast<std::size_t>(scene.stepsPerBar)))) {
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

        widgets.sampleCheck->blockSignals(true);
        widgets.soundfontCheck->blockSignals(true);
        widgets.midiCheck->blockSignals(true);
        widgets.midiChannelSpin->blockSignals(true);
        widgets.soundfontChannelSpin->blockSignals(true);
        widgets.soundfontBankSpin->blockSignals(true);
        widgets.soundfontProgramSpin->blockSignals(true);
        widgets.soundfontPresetCombo->blockSignals(true);

        populatePresetCombo(widgets.soundfontPresetCombo, presets);

        widgets.sampleCheck->setChecked(instrument.layers.sampleEnabled);
        widgets.soundfontCheck->setChecked(instrument.layers.soundfontEnabled);
        widgets.midiCheck->setChecked(instrument.layers.midiEnabled);
        widgets.midiChannelSpin->setValue(instrument.layers.midiChannel);
        widgets.soundfontChannelSpin->setValue(instrument.layers.soundfontChannel);
        widgets.soundfontBankSpin->setValue(instrument.layers.soundfontBank);
        widgets.soundfontProgramSpin->setValue(instrument.layers.soundfontProgram);
        widgets.soundfontPresetCombo->setCurrentIndex(
            comboIndexForPreset(widgets.soundfontPresetCombo, instrument.layers.soundfontBank, instrument.layers.soundfontProgram));

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

    updateStepGridButtonSizing();
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
    previewPlayButton_->setEnabled(previewPath_.isEmpty() == false);
    previewStopButton_->setEnabled(previewPath_.isEmpty() == false);
    previewRewindButton_->setEnabled(previewPath_.isEmpty() == false);
    previewForwardButton_->setEnabled(previewPath_.isEmpty() == false);

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

void MainWindow::syncActiveBarToTransport() {
    if (controller_->isPlaying() == false) {
        return;
    }

    const GrooveScene scene = controller_->scene();
    const int currentStep = controller_->currentStep();
    if ((currentStep < 0) || (scene.stepsPerBar <= 0)) {
        return;
    }

    const int activeBar = std::clamp((currentStep / scene.stepsPerBar) + 1, 1, scene.patternBars);
    if (editBarSpin_->value() == activeBar) {
        return;
    }

    editBarSpin_->blockSignals(true);
    editBarSpin_->setValue(activeBar);
    editBarSpin_->blockSignals(false);
}

void MainWindow::saveProject() {
    const QString defaultName = QString("%1/groove-forge-project-%2.json")
        .arg(QDir::homePath())
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss"));
    const QString path = QFileDialog::getSaveFileName(this, "Save Project", defaultName, "Groove Forge Project (*.json)");
    if (path.isEmpty()) {
        return;
    }

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate) == false) {
        QMessageBox::warning(this, "Save Failed", "Could not write the project file.");
        return;
    }

    QJsonObject root;
    root["format"] = "groove-forge-project";
    root["version"] = 1;
    root["scene"] = sceneToJson(controller_->scene());
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();

    lastMessage_ = QString("Saved project %1").arg(QFileInfo(path).fileName());
    refreshFromScene();
}

void MainWindow::loadProject() {
    const QString path = QFileDialog::getOpenFileName(this, "Load Project", QString(), "Groove Forge Project (*.json)");
    if (path.isEmpty()) {
        return;
    }

    QFile file(path);
    if (file.open(QIODevice::ReadOnly) == false) {
        QMessageBox::warning(this, "Load Failed", "Could not read the project file.");
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (document.isObject() == false) {
        QMessageBox::warning(this, "Load Failed", "The selected file is not a valid Groove Forge project.");
        return;
    }

    const QJsonObject root = document.object();
    if (root["format"].toString() != "groove-forge-project") {
        QMessageBox::warning(this, "Load Failed", "The selected file is not a Groove Forge project.");
        return;
    }
    if (root["version"].toInt() != 1) {
        QMessageBox::warning(this, "Load Failed", "This project file version is not supported by the current build.");
        return;
    }

    GrooveScene scene = sceneFromJson(root["scene"].toObject());
    controller_->setScene(scene);

    QStringList warnings;
    if (scene.soundfontPath.empty() == false && controller_->loadSoundfont(scene.soundfontPath) == false) {
        warnings << "SoundFont could not be reopened";
    }

    const GrooveScene loadedScene = controller_->scene();
    for (int instrumentIndex = 0; instrumentIndex < static_cast<int>(loadedScene.instruments.size()); ++instrumentIndex) {
        const auto& instrument = loadedScene.instruments[static_cast<std::size_t>(instrumentIndex)];
        if (instrument.layers.samplePath.empty() == false) {
            if (controller_->loadSample(instrumentIndex, instrument.layers.samplePath) == false) {
                warnings << QString("Sample missing for %1").arg(QString::fromStdString(instrument.name));
            }
        }
    }

    lastMessage_ = QString("Loaded project %1").arg(QFileInfo(path).fileName());
    if (warnings.isEmpty() == false) {
        QMessageBox::warning(this, "Project Loaded With Warnings", warnings.join("\n"));
    }
    refreshFromScene();
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

void MainWindow::setPreviewFile(const QString& path) {
    previewPath_ = path;
    if (previewPath_.isEmpty()) {
        previewPlayer_->stop();
        previewPlayer_->setMedia(QMediaContent());
        previewFileLabel_->setText("No preview file loaded");
        return;
    }

    previewPlayer_->stop();
    previewPlayer_->setMedia(QMediaContent(QUrl::fromLocalFile(previewPath_)));
    previewFileLabel_->setText(QFileInfo(previewPath_).fileName());
}

void MainWindow::seekPreview(qint64 deltaMs) {
    if (previewPath_.isEmpty()) {
        return;
    }
    const qint64 newPosition = std::max<qint64>(0, previewPlayer_->position() + deltaMs);
    previewPlayer_->setPosition(newPosition);
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

    setPreviewFile(path);
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

    setPreviewFile(path);
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

    setPreviewFile(path);
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
    button->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(button, &QWidget::customContextMenuRequested, this, [this, instrumentIndex, stepIndex](const QPoint&) {
        editStepParameters(instrumentIndex, stepIndex);
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

void MainWindow::editStepParameters(int instrumentIndex, int stepIndex) {
    const GrooveScene scene = controller_->scene();
    const int absoluteStep = currentEditBarIndex() * scene.stepsPerBar + stepIndex;
    if ((instrumentIndex < 0) || (instrumentIndex >= static_cast<int>(scene.instruments.size()))) {
        return;
    }
    if ((absoluteStep < 0) || (absoluteStep >= totalStepCount(scene))) {
        return;
    }

    const Step currentStep = scene.instruments[static_cast<std::size_t>(instrumentIndex)].steps[static_cast<std::size_t>(absoluteStep)];

    QDialog dialog(this);
    dialog.setWindowTitle(QString("Step %1 Settings").arg(stepIndex + 1));
    auto* form = new QFormLayout(&dialog);

    auto* velocitySpin = new QDoubleSpinBox(&dialog);
    velocitySpin->setRange(0.0, 1.0);
    velocitySpin->setDecimals(2);
    velocitySpin->setSingleStep(0.05);
    velocitySpin->setValue(currentStep.velocity);

    auto* noteSpin = new QSpinBox(&dialog);
    noteSpin->setRange(0, 127);
    noteSpin->setValue(currentStep.note);

    auto* attackSpin = new QDoubleSpinBox(&dialog);
    attackSpin->setRange(0.001, 2.0);
    attackSpin->setDecimals(3);
    attackSpin->setSingleStep(0.005);
    attackSpin->setValue(currentStep.attack);

    auto* decaySpin = new QDoubleSpinBox(&dialog);
    decaySpin->setRange(0.001, 4.0);
    decaySpin->setDecimals(3);
    decaySpin->setSingleStep(0.010);
    decaySpin->setValue(currentStep.decay);

    auto* sustainSpin = new QDoubleSpinBox(&dialog);
    sustainSpin->setRange(0.0, 1.0);
    sustainSpin->setDecimals(2);
    sustainSpin->setSingleStep(0.05);
    sustainSpin->setValue(currentStep.sustain);

    auto* releaseSpin = new QDoubleSpinBox(&dialog);
    releaseSpin->setRange(0.001, 4.0);
    releaseSpin->setDecimals(3);
    releaseSpin->setSingleStep(0.010);
    releaseSpin->setValue(currentStep.release);

    auto* gateSpin = new QDoubleSpinBox(&dialog);
    gateSpin->setRange(0.05, 1.5);
    gateSpin->setDecimals(2);
    gateSpin->setSingleStep(0.05);
    gateSpin->setValue(currentStep.gate);

    form->addRow("Velocity", velocitySpin);
    form->addRow("Note", noteSpin);
    form->addRow("Attack", attackSpin);
    form->addRow("Decay", decaySpin);
    form->addRow("Sustain", sustainSpin);
    form->addRow("Release", releaseSpin);
    form->addRow("Gate", gateSpin);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    GrooveScene updatedScene = scene;
    auto& step = updatedScene.instruments[static_cast<std::size_t>(instrumentIndex)].steps[static_cast<std::size_t>(absoluteStep)];
    step.active = true;
    step.velocity = static_cast<float>(velocitySpin->value());
    step.note = noteSpin->value();
    step.attack = static_cast<float>(attackSpin->value());
    step.decay = static_cast<float>(decaySpin->value());
    step.sustain = static_cast<float>(sustainSpin->value());
    step.release = static_cast<float>(releaseSpin->value());
    step.gate = static_cast<float>(gateSpin->value());
    controller_->setScene(updatedScene);
    lastMessage_ = QString("Updated step %1 for %2")
        .arg(stepIndex + 1)
        .arg(QString::fromStdString(updatedScene.instruments[static_cast<std::size_t>(instrumentIndex)].name));
    refreshFromScene();
}

QString MainWindow::soundfontLabelText(const std::string& path) const {
    if (path.empty()) {
        return "No soundfont loaded";
    }
    return QString("SF2 %1").arg(QFileInfo(QString::fromStdString(path)).fileName());
}

}  // namespace groove
