#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QRandomGenerator>
#include <QLabel>
#include <QDoubleSpinBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateWorld);

    spawnObjects();
}

MainWindow::~MainWindow()
{
    qDeleteAll(objects);
    objects.clear();
}

void MainWindow::setupUi() {
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    QVBoxLayout *controlLayout = new QVBoxLayout();

    scene = new QGraphicsScene(0, 0, 500, 500, this);
    view = new QGraphicsView(scene);
    view->setRenderHint(QPainter::Antialiasing);
    view->setFixedSize(520, 520);

    QGroupBox *settingsGroup = new QGroupBox("Параметры симуляции");
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsGroup);

    sbHumanCount = new QSpinBox();
    sbHumanCount->setRange(0, 500);
    sbHumanCount->setValue(50);
    settingsLayout->addWidget(new QLabel("Люди (кол-во):"));
    settingsLayout->addWidget(sbHumanCount);

    sbZombieCount = new QSpinBox();
    sbZombieCount->setRange(0, 100);
    sbZombieCount->setValue(5);
    settingsLayout->addWidget(new QLabel("Зомби (кол-во):"));
    settingsLayout->addWidget(sbZombieCount);

    QSpinBox *sbSearchRadius = new QSpinBox();
    sbSearchRadius->setRange(10, 300);
    sbSearchRadius->setValue(150);
    sbSearchRadius->setObjectName("sbSearchRadius"); // Для доступа позже
    settingsLayout->addWidget(new QLabel("Радиус поиска/обнаружения:"));
    settingsLayout->addWidget(sbSearchRadius);

    QSpinBox *sbSafetyRadius = new QSpinBox();
    sbSafetyRadius->setRange(5, 100);
    sbSafetyRadius->setValue(25);
    sbSafetyRadius->setObjectName("sbSafetyRadius");
    settingsLayout->addWidget(new QLabel("Зона безопасности (уклонение):"));
    settingsLayout->addWidget(sbSafetyRadius);

    QDoubleSpinBox *sbMaxSpeed = new QDoubleSpinBox();
    sbMaxSpeed->setRange(0.5, 10.0);
    sbMaxSpeed->setValue(2.5);
    sbMaxSpeed->setSingleStep(0.5);
    sbMaxSpeed->setObjectName("sbMaxSpeed");
    settingsLayout->addWidget(new QLabel("Максимальная скорость:"));
    settingsLayout->addWidget(sbMaxSpeed);

    btnSpawn = new QPushButton("Сброс / Создать");
    connect(btnSpawn, &QPushButton::clicked, this, &MainWindow::onSpawnClicked);
    settingsLayout->addWidget(btnSpawn);

    statsLabel = new QLabel("Статистика:\nЛюди: 0\nЗомби: 0");
    QFont font = statsLabel->font();
    font.setPointSize(11);
    font.setBold(true);
    statsLabel->setFont(font);

    btnStart = new QPushButton("СТАРТ");
    btnStart->setStyleSheet("background-color: #e1ffdc;");
    btnStop = new QPushButton("СТОП");
    btnStop->setStyleSheet("background-color: #ffdcdc;");

    connect(btnStart, &QPushButton::clicked, this, &MainWindow::startSimulation);
    connect(btnStop, &QPushButton::clicked, this, &MainWindow::stopSimulation);

    // 4. График
    customPlot = new QCustomPlot();
    customPlot->setMinimumHeight(200);
    customPlot->legend->setVisible(true);
    customPlot->addGraph();
    customPlot->graph(0)->setPen(QPen(Qt::blue));
    customPlot->graph(0)->setName("Люди");
    customPlot->addGraph();
    customPlot->graph(1)->setPen(QPen(Qt::darkGreen));
    customPlot->graph(1)->setName("Зомби");


    controlLayout->addWidget(settingsGroup);
    controlLayout->addWidget(statsLabel);
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(btnStart);
    btnLayout->addWidget(btnStop);
    controlLayout->addLayout(btnLayout);
    controlLayout->addWidget(customPlot);
    controlLayout->addStretch();

    mainLayout->addWidget(view);
    mainLayout->addLayout(controlLayout);

    QGroupBox *forceGroup = new QGroupBox("Внешние силы");
    QVBoxLayout *forceLayout = new QVBoxLayout(forceGroup);

    QCheckBox *cbPetri = new QCheckBox("Режим 'Чашка Петри'");
    forceLayout->addWidget(cbPetri);

    QLabel *lWind = new QLabel("Сила ветра (X):");
    QDoubleSpinBox *sbWindX = new QDoubleSpinBox();
    sbWindX->setRange(-2.0, 2.0);
    forceLayout->addWidget(lWind);
    forceLayout->addWidget(sbWindX);

    connect(cbPetri, &QCheckBox::toggled, [](bool checked){
        WorldObject::usePetriMode = checked;
    });
    connect(sbWindX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [](double val){
        WorldObject::windX = val;
    });

    connect(cbPetri, &QCheckBox::toggled, [this](bool checked){
        WorldObject::usePetriMode = checked;
        updatePetriVisuals();
    });

    connect(slAlpha, &QSlider::valueChanged, [this](int val){
        WorldObject::alpha = val * M_PI / 180.0;
        updatePetriVisuals();
    });

    settingsLayout->addWidget(forceGroup);
}

void MainWindow::onSpawnClicked() {
    stopSimulation();
    spawnObjects();
}

void MainWindow::spawnObjects() {
    timer->stop();

    objects.clear();

    scene->clear();


    simTime = 0;
    timeData.clear();
    humanData.clear();
    zombieData.clear();

    int humanCount = sbHumanCount->value();
    int zombieCount = sbZombieCount->value();

    int searchRad = findChild<QSpinBox*>("sbSearchRadius")->value();
    int safetyRad = findChild<QSpinBox*>("sbSafetyRadius")->value();
    double maxSpeed = findChild<QDoubleSpinBox*>("sbMaxSpeed")->value();

    for(int i=0; i<humanCount; ++i) {
        Human *h = new Human();
        h->setPosition(QRandomGenerator::global()->bounded(500),
                       QRandomGenerator::global()->bounded(500));

        h->_obsRadius = searchRad;
        h->_safetyRadius = safetyRad;
        h->_maxSpeed = maxSpeed;

        h->_curState.vel = {(QRandomGenerator::global()->bounded(10)-5.0)/2.0,
                            (QRandomGenerator::global()->bounded(10)-5.0)/2.0};

        scene->addItem(h);
        objects.append(h);
    }

    for(int i=0; i<zombieCount; ++i) {
        Zombie *z = new Zombie();
        z->setPosition(QRandomGenerator::global()->bounded(500),
                       QRandomGenerator::global()->bounded(500));

        z->_searchRadius = searchRad;
        z->_safetyRadius = safetyRad;
        z->_maxSpeed = maxSpeed;

        z->_curState.vel = {(QRandomGenerator::global()->bounded(10)-5.0)/2.0,
                            (QRandomGenerator::global()->bounded(10)-5.0)/2.0};

        scene->addItem(z);
        objects.append(z);
    }

    statsLabel->setText(QString("Статистика:\nЛюди: %1\nЗомби: %2")
                            .arg(humanCount).arg(zombieCount));
    updateGraph();
}

void MainWindow::startSimulation() {
    if(!timer->isActive()) timer->start(30);
}

void MainWindow::stopSimulation() {
    timer->stop();
    updateGraph();
}

void MainWindow::updateWorld() {
    int humans = 0;
    int zombies = 0;
    double dt = 1.0;
    simTime += dt;

    for(auto obj : objects) {
        obj->updateState(objects, dt); //
        obj->update();

        if(obj->getType() == HUMAN) humans++;
        else zombies++;
    }

    timeData.push_back(simTime);
    humanData.push_back(humans);
    zombieData.push_back(zombies);

    statsLabel->setText(QString("Статистика:\nЛюди: %1\nЗомби: %2")
                            .arg(humans).arg(zombies));

    scene->update();

    if((int)simTime % 10 == 0) updateGraph();
}

void MainWindow::updateGraph() {
    customPlot->graph(0)->setData(timeData, humanData);
    customPlot->graph(1)->setData(timeData, zombieData);
    customPlot->rescaleAxes();
    customPlot->replot();
}

void MainWindow::updatePetriVisuals() {
    if (!petriCircle) {
        petriCircle = scene->addEllipse(0, 0, 500, 500, QPen(Qt::gray, 2, Qt::DashLine));
        petriCircle->setZValue(-1); // Чтобы была под агентами
    }
    if (!gravityVector) {
        gravityVector = scene->addLine(250, 250, 250, 250, QPen(Qt::red, 3));
        gravityVector->setZValue(1); // Поверх всего
    }

    bool isVisible = WorldObject::usePetriMode;
    petriCircle->setVisible(isVisible);
    gravityVector->setVisible(isVisible);

    if (isVisible) {
        double len = 60.0;
        double ex = 250 + std::cos(WorldObject::alpha) * len;
        double ey = 250 + std::sin(WorldObject::alpha) * len;
        gravityVector->setLine(250, 250, ex, ey);
    }
}

