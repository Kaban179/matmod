#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
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

    // Layouts
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    QVBoxLayout *controlLayout = new QVBoxLayout();

    scene = new QGraphicsScene(0, 0, 500, 500, this);
    view = new QGraphicsView(scene);
    view->setRenderHint(QPainter::Antialiasing);
    view->setFixedSize(520, 520);

    statsLabel = new QLabel("Статистика:\nЛюди: 0\nЗомби: 0");
    QFont font = statsLabel->font();
    font.setPointSize(12);
    font.setBold(true);
    statsLabel->setFont(font);

    btnStart = new QPushButton("Старт");
    btnStop = new QPushButton("Стоп");

    connect(btnStart, &QPushButton::clicked, this, &MainWindow::startSimulation);
    connect(btnStop, &QPushButton::clicked, this, &MainWindow::stopSimulation);
    controlLayout->addWidget(statsLabel);
    controlLayout->addStretch(); // Пружина, чтобы кнопки были внизу или текст вверху
    controlLayout->addWidget(btnStart);
    controlLayout->addWidget(btnStop);

    mainLayout->addWidget(view);
    mainLayout->addLayout(controlLayout);
}

void MainWindow::spawnObjects() {
    scene->clear();
    qDeleteAll(objects);
    objects.clear();

    // Генерация людей
    for(int i=0; i<HUMAN_COUNT; ++i) {
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

    for(int i=0; i<ZOMBIE_COUNT; ++i) {
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
                            .arg(HUMAN_COUNT).arg(ZOMBIE_COUNT));
}

void MainWindow::startSimulation() {
    if(!timer->isActive()) {
        timer->start(30);
    }
}

void MainWindow::stopSimulation() {
    timer->stop();
}

void MainWindow::updateWorld() {
    int humans = 0;
    int zombies = 0;

    for(auto obj : objects) {
        obj->updateState(objects, 1.0);
        obj->update();

        if(obj->getType() == HUMAN) humans++;
        else zombies++;
    }


    statsLabel->setText(QString("Статистика:\nЛюди: %1\nЗомби: %2")
                            .arg(humans).arg(zombies));

    scene->update();
}
