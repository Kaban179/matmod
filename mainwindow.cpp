#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QRandomGenerator>

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

    // 1
    scene = new QGraphicsScene(0, 0, 500, 500, this);
    view = new QGraphicsView(scene);
    view->setRenderHint(QPainter::Antialiasing);
    view->setFixedSize(520, 520);

    // 2
    QGroupBox *settingsGroup = new QGroupBox("Настройки популяции");
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsGroup);

    QLabel *lH = new QLabel("Люди:");
    sbHumanCount = new QSpinBox();
    sbHumanCount->setRange(0, 500);
    sbHumanCount->setValue(50);

    QLabel *lZ = new QLabel("Зомби:");
    sbZombieCount = new QSpinBox();
    sbZombieCount->setRange(0, 100);
    sbZombieCount->setValue(3);

    btnSpawn = new QPushButton("Сброс / Создать");
    connect(btnSpawn, &QPushButton::clicked, this, &MainWindow::onSpawnClicked);

    settingsLayout->addWidget(lH);
    settingsLayout->addWidget(sbHumanCount);
    settingsLayout->addWidget(lZ);
    settingsLayout->addWidget(sbZombieCount);
    settingsLayout->addWidget(btnSpawn);

    //----
    statsLabel = new QLabel("Статистика:\nЛюди: 0\nЗомби: 0");
    QFont font = statsLabel->font();
    font.setPointSize(12);
    font.setBold(true);
    statsLabel->setFont(font);

    btnStart = new QPushButton("Старт");
    btnStop = new QPushButton("Стоп");

    connect(btnStart, &QPushButton::clicked, this, &MainWindow::startSimulation);
    connect(btnStop, &QPushButton::clicked, this, &MainWindow::stopSimulation);

    //----
    customPlot = new QCustomPlot();
    customPlot->setMinimumHeight(200);
    customPlot->legend->setVisible(true);

    customPlot->addGraph();
    customPlot->graph(0)->setPen(QPen(Qt::blue));
    customPlot->graph(0)->setName("Люди");

    customPlot->addGraph();
    customPlot->graph(1)->setPen(QPen(Qt::darkGreen));
    customPlot->graph(1)->setName("Зомби");

    customPlot->xAxis->setLabel("Время (усл. ед.)");
    customPlot->yAxis->setLabel("Количество");

    // Сборка ------
    controlLayout->addWidget(settingsGroup);
    controlLayout->addWidget(statsLabel);
    controlLayout->addWidget(btnStart);
    controlLayout->addWidget(btnStop);
    controlLayout->addWidget(customPlot);
    controlLayout->addStretch();

    mainLayout->addWidget(view);
    mainLayout->addLayout(controlLayout);
}

void MainWindow::onSpawnClicked() {
    stopSimulation();
    spawnObjects();
}

void MainWindow::spawnObjects() {
    scene->clear();
    //qDeleteAll(objects);
    objects.clear();


    simTime = 0;
    timeData.clear();
    humanData.clear();
    zombieData.clear();

    // -------------
    int humanCount = sbHumanCount->value();
    int zombieCount = sbZombieCount->value();

    // Генерация людей
    for(int i=0; i<humanCount; ++i) {
        Human *h = new Human();
        double x = QRandomGenerator::global()->bounded(500);
        double y = QRandomGenerator::global()->bounded(500);
        double vx = (QRandomGenerator::global()->bounded(20) - 10) / 5.0;
        double vy = (QRandomGenerator::global()->bounded(20) - 10) / 5.0;

        h->setPosition(x, y);
        h->_curState.vel = {vx, vy};

        scene->addItem(h);
        objects.append(h);
    }

    // Генерация зомби
    for(int i=0; i<zombieCount; ++i) {
        Zombie *z = new Zombie();
        double x = QRandomGenerator::global()->bounded(500);
        double y = QRandomGenerator::global()->bounded(500);
        double vx = (QRandomGenerator::global()->bounded(20) - 10) / 4.0;
        double vy = (QRandomGenerator::global()->bounded(20) - 10) / 4.0;

        z->setPosition(x, y);
        z->_curState.vel = {vx, vy};

        scene->addItem(z);
        objects.append(z);
    }

    statsLabel->setText(QString("Статистика:\nЛюди: %1\nЗомби: %2")
                            .arg(humanCount).arg(zombieCount));

    customPlot->replot();
}

void MainWindow::startSimulation() {
    if(!timer->isActive()) {
        timer->start(30);
    }
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
        obj->updateState(objects, dt);
        obj->update();

        // Подсчет
        if(obj->getType() == HUMAN) humans++;
        else zombies++;
    }

    //  -------
    timeData.push_back(simTime);
    humanData.push_back(humans);
    zombieData.push_back(zombies);

    statsLabel->setText(QString("Статистика:\nЛюди: %1\nЗомби: %2")
                            .arg(humans).arg(zombies));

    scene->update();

    // Если нужно обновление графика в реальном времени, раскомментируйте:
    // updateGraph();
}

void MainWindow::updateGraph() {
    // Устанавливаем данные
    customPlot->graph(0)->setData(timeData, humanData); // Люди
    customPlot->graph(1)->setData(timeData, zombieData); // Зомби

    // Масштабируем оси под данные
    customPlot->rescaleAxes();
    customPlot->replot();
}
