#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QRandomGenerator>
#include <QLabel>
#include <QDoubleSpinBox> // Добавлено для дробных чисел (скорость)

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
    // Очистка списка объектов при закрытии
    qDeleteAll(objects);
    objects.clear();
}

void MainWindow::setupUi() {
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    QVBoxLayout *controlLayout = new QVBoxLayout();

    // 1. Настройка графической сцены
    scene = new QGraphicsScene(0, 0, 500, 500, this);
    view = new QGraphicsView(scene);
    view->setRenderHint(QPainter::Antialiasing);
    view->setFixedSize(520, 520);

    // 2. Группа настроек популяции и ПОВЕДЕНИЯ
    QGroupBox *settingsGroup = new QGroupBox("Параметры симуляции");
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsGroup);

    // Количество
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

    // --- НОВЫЕ ПАРАМЕТРЫ ПОВЕДЕНИЯ ---

    // Радиус поиска/наблюдения (1.1 и 2.1 по заданию)
    QSpinBox *sbSearchRadius = new QSpinBox();
    sbSearchRadius->setRange(10, 300);
    sbSearchRadius->setValue(150);
    sbSearchRadius->setObjectName("sbSearchRadius"); // Для доступа позже
    settingsLayout->addWidget(new QLabel("Радиус поиска/обнаружения:"));
    settingsLayout->addWidget(sbSearchRadius);

    // Радиус безопасности (1.2 и 2.2 по заданию)
    QSpinBox *sbSafetyRadius = new QSpinBox();
    sbSafetyRadius->setRange(5, 100);
    sbSafetyRadius->setValue(25);
    sbSafetyRadius->setObjectName("sbSafetyRadius");
    settingsLayout->addWidget(new QLabel("Зона безопасности (уклонение):"));
    settingsLayout->addWidget(sbSafetyRadius);

    // Максимальная скорость (v_max из задания)
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

    // 3. Статистика и кнопки управления
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

    // Сборка интерфейса
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
}

void MainWindow::onSpawnClicked() {
    stopSimulation();
    spawnObjects();
}

void MainWindow::spawnObjects() {
    timer->stop(); // Обязательно останавливаем таймер перед очисткой!

    // 1. Очищаем список, НЕ удаляя объекты вручную,
    // так как scene->clear() сделает это сам за нас.
    objects.clear();

    // 2. Очищаем сцену (это удалит объекты из памяти)
    scene->clear();


    simTime = 0;
    timeData.clear();
    humanData.clear();
    zombieData.clear();

    // Получаем значения параметров из UI
    int humanCount = sbHumanCount->value();
    int zombieCount = sbZombieCount->value();

    // Ищем наши новые поля по именам
    int searchRad = findChild<QSpinBox*>("sbSearchRadius")->value();
    int safetyRad = findChild<QSpinBox*>("sbSafetyRadius")->value();
    double maxSpeed = findChild<QDoubleSpinBox*>("sbMaxSpeed")->value();

    // Создание Людей
    for(int i=0; i<humanCount; ++i) {
        Human *h = new Human();
        h->setPosition(QRandomGenerator::global()->bounded(500),
                       QRandomGenerator::global()->bounded(500));

        // Передаем параметры логики (уклонение от зомби и своих)
        h->_obsRadius = searchRad;    // Зона наблюдения
        h->_safetyRadius = safetyRad; // Зона безопасности
        h->_maxSpeed = maxSpeed;      // v_max

        // Начальная случайная скорость
        h->_curState.vel = {(QRandomGenerator::global()->bounded(10)-5.0)/2.0,
                            (QRandomGenerator::global()->bounded(10)-5.0)/2.0};

        scene->addItem(h);
        objects.append(h);
    }

    // Создание Зомби
    for(int i=0; i<zombieCount; ++i) {
        Zombie *z = new Zombie();
        z->setPosition(QRandomGenerator::global()->bounded(500),
                       QRandomGenerator::global()->bounded(500));

        // Передаем параметры логики (поиск целей и уклонение от своих)
        z->_searchRadius = searchRad; // Зона поиска
        z->_safetyRadius = safetyRad; // Зона безопасности
        z->_maxSpeed = maxSpeed;      // v_max

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
    double dt = 1.0; // Шаг времени
    simTime += dt;

    for(auto obj : objects) {
        // Вызов обновленной логики (поиск целей/уклонение)
        obj->updateState(objects, dt); //
        obj->update();

        if(obj->getType() == HUMAN) humans++;
        else zombies++;
    }

    // Сохранение данных для графика
    timeData.push_back(simTime);
    humanData.push_back(humans);
    zombieData.push_back(zombies);

    statsLabel->setText(QString("Статистика:\nЛюди: %1\nЗомби: %2")
                            .arg(humans).arg(zombies));

    scene->update();

    // Опционально: обновление графика каждые 10 кадров для производительности
    if((int)simTime % 10 == 0) updateGraph();
}

void MainWindow::updateGraph() {
    customPlot->graph(0)->setData(timeData, humanData);
    customPlot->graph(1)->setData(timeData, zombieData);
    customPlot->rescaleAxes();
    customPlot->replot();
}
