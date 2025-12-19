#ifndef WORLDOBJECT_H
#define WORLDOBJECT_H

#include <QObject>
#include <QGraphicsItem>
#include <QPainter>
#include <vector>


enum objType {
    HUMAN,
    ZOMBIE
};

enum objStatus {
    NORMAL,
    PANIC,
    AGGRESSIVE
};

struct objState {
    objStatus _curStatus;
    std::vector<double> pos; // x, y
    std::vector<double> vel; // vx, vy
};


class WorldObject : public QObject, public QGraphicsItem
{
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

public:
    explicit WorldObject(QObject *parent = nullptr);
    virtual ~WorldObject() {}

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    virtual void updateState(const QList<WorldObject*>& neighbors, double dt) = 0;

    objType getType() const { return _type; }
    objState getState() const { return _curState; }

    void setPosition(double x, double y);

public:
    objType _type;
    objState _curState;
    double _radius; // Радиус отрисовки

    // Новые параметры
    double _maxSpeed = 2.0;
    double _safetyRadius = 20.0;
    double _searchRadius = 150.0; // Для зомби
    double _obsRadius = 100.0;    // Для людей
    double _biteRadius = 10.0;    // Дистанция укуса

    // Вспомогательный метод для нормализации вектора
    void applySteering(double targetX, double targetY, double weight, bool flee = false);
};

// --- Класс Человека ---
class Human : public WorldObject
{
    Q_OBJECT
public:
    explicit Human(QObject *parent = nullptr);

    void updateState(const QList<WorldObject*>& neighbors, double dt) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

public slots:
    void biteSlot();
};
class Zombie : public WorldObject
{
    Q_OBJECT
public:
    explicit Zombie(QObject *parent = nullptr);

    void updateState(const QList<WorldObject*>& neighbors, double dt) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

signals:
    void biteSignal();

public:
    double _biteRadius;
    bool _isBusy;
    int _cooldownTimer;
};

#endif // WORLDOBJECT_H
