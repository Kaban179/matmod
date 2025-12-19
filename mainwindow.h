#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVector>
#include "qcustomplot.h" // Добавлено для графиков
#include "worldobject.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateWorld();
    void startSimulation();
    void stopSimulation();
    void onSpawnClicked();

private:
    void setupUi();
    void spawnObjects();
    void updateGraph();
    QWidget *centralWidget;

    // Визуализация мира
    QGraphicsView *view;
    QGraphicsScene *scene;

    // Элементы управления и вывода
    QLabel *statsLabel;
    QCustomPlot *customPlot; // Виджет графика

    // Элементы настройки
    QSpinBox *sbHumanCount;
    QSpinBox *sbZombieCount;
    QPushButton *btnSpawn; // Кнопка "Применить настройки / Сброс"

    QPushButton *btnStart;
    QPushButton *btnStop;

    QTimer *timer;
    QList<WorldObject*> objects;

    // Данные для статистики
    double simTime; // Текущее время симуляции
    QVector<double> timeData;
    QVector<double> humanData;
    QVector<double> zombieData;
    QSpinBox *sbSafetyRadius;
    QSpinBox *sbSearchRadius;
    QDoubleSpinBox *sbMaxSpeed;
};

#endif // MAINWINDOW_H
