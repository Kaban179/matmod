#include "worldobject.h"
#include <QRandomGenerator>
#include <cmath>
#include <QDebug>

// --- WorldObject ---

WorldObject::WorldObject(QObject *parent) : QObject(parent) {
    _radius = 5.0;
    _curState.pos = {0, 0};
    _curState.vel = {0, 0};
}

QRectF WorldObject::boundingRect() const {
    return QRectF(-_radius, -_radius, _radius*2, _radius*2);
}

void WorldObject::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option); Q_UNUSED(widget);
    painter->drawEllipse(boundingRect());
}

void WorldObject::setPosition(double x, double y) {
    _curState.pos[0] = x;
    _curState.pos[1] = y;
    setPos(x, y);
}

// --- Human ---

Human::Human(QObject *parent) : WorldObject(parent) {
    _type = HUMAN;
    _curState._curStatus = NORMAL;
}


void Human::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option); Q_UNUSED(widget);

    if (_type == ZOMBIE) {
        painter->setBrush(Qt::darkGreen);
    } else {
        painter->setBrush(Qt::blue);
    }

    painter->setPen(Qt::black);
    painter->drawEllipse(boundingRect());
}

void Human::updateState(const QList<WorldObject *> &neighbors, double dt) {
    // 1. ПРОВЕРКА: Если человек стал зомби, он должен атаковать
    if (this->_type == ZOMBIE) {
        WorldObject* closestVictim = nullptr;
        double minDistSq = _searchRadius * _searchRadius;

        for (WorldObject* obj : neighbors) {
            if (obj == this || obj->getType() == ZOMBIE) continue;

            double dx = obj->getState().pos[0] - _curState.pos[0];
            double dy = obj->getState().pos[1] - _curState.pos[1];
            double dSq = dx*dx + dy*dy; // Рассчитываем квадрат дистанции

            if (dSq < minDistSq) {
                minDistSq = dSq;
                closestVictim = obj;
            }
        }

        if (closestVictim) {
            double dx = closestVictim->getState().pos[0] - _curState.pos[0];
            double dy = closestVictim->getState().pos[1] - _curState.pos[1];
            double actualDist = std::sqrt(minDistSq); // Теперь безопасно берем корень

            // Вектор скорости к цели (v_max * направление)
            _curState.vel[0] = (dx / actualDist) * _maxSpeed;
            _curState.vel[1] = (dy / actualDist) * _maxSpeed;

            // Логика укуса: если догнали — заражаем
            if (actualDist < 10.0) { // 10.0 - дистанция укуса
                Human* h = qobject_cast<Human*>(closestVictim);
                if (h) h->biteSlot();
            }
        }
    }
    else {
        // 2. ЛОГИКА ЧЕЛОВЕКА (Убегание)
        double targetVx = 0;
        double targetVy = 0;
        bool fleeing = false;

        for (WorldObject* obj : neighbors) {
            if (obj == this) continue;

            double dx = obj->getState().pos[0] - _curState.pos[0];
            double dy = obj->getState().pos[1] - _curState.pos[1];
            double dSq = dx*dx + dy*dy;

            // Если зомби в радиусе наблюдения
            if (obj->getType() == ZOMBIE && dSq < (_obsRadius * _obsRadius)) {
                double dist = std::sqrt(dSq);
                // Убегаем в противоположную сторону (-dx, -dy)
                targetVx -= (dx / dist) * _maxSpeed;
                targetVy -= (dy / dist) * _maxSpeed;
                fleeing = true;
            }
        }

        if (fleeing) {
            _curState.vel[0] = targetVx;
            _curState.vel[1] = targetVy;
        }
    }

    // 3. ОБЩИЙ БЛОК: Движение и отскок от стен
    double nextX = _curState.pos[0] + _curState.vel[0] * dt;
    double nextY = _curState.pos[1] + _curState.vel[1] * dt;

    if (nextX <= 0 || nextX >= 500) { _curState.vel[0] *= -1; nextX = qBound(0.0, nextX, 500.0); }
    if (nextY <= 0 || nextY >= 500) { _curState.vel[1] *= -1; nextY = qBound(0.0, nextY, 500.0); }

    setPosition(nextX, nextY);
}




void Human::biteSlot() {
    _type = ZOMBIE; // Меняем тип
    // Можно добавить смену цвета сразу здесь, если нужно
    update(); // Перерисовываем
}

// --- Zombie ---

Zombie::Zombie(QObject *parent) : WorldObject(parent) {
    _type = ZOMBIE;
    _curState._curStatus = AGGRESSIVE;
    _biteRadius = 15.0; // Радиус укуса
    _isBusy = false;
    _cooldownTimer = 0;
}

void Zombie::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    painter->setBrush(Qt::darkGreen);
    painter->setPen(Qt::black);
    painter->drawEllipse(boundingRect());
}

void Zombie::updateState(const QList<WorldObject *> &neighbors, double dt) {
    // 1. Проверка кулдауна после укуса (чтобы не «прилипал» мгновенно)
    if (_isBusy) {
        _cooldownTimer++;
        if (_cooldownTimer > 30) {
            _isBusy = false;
            _cooldownTimer = 0;
        }
    }

    double targetVx = _curState.vel[0];
    double targetVy = _curState.vel[1];

    WorldObject* closestVictim = nullptr;
    double minDist = _searchRadius;

    double searchRadSq = _searchRadius * _searchRadius;
    double safetyRadSq = _safetyRadius * _safetyRadius;

    for (WorldObject* obj : neighbors) {
        if (obj == this) continue;

        double dx = obj->getState().pos[0] - _curState.pos[0];
        double dy = obj->getState().pos[1] - _curState.pos[1];
        double dSq = dx*dx + dy*dy;
        if (dSq < searchRadSq) {
            // И ТОЛЬКО ЕСЛИ объект попал в радиус, вычисляем корень для векторов
            double dist = std::sqrt(dSq);


            if (dist < 0.1) continue;


        // 1.1 и 1.3: Поиск ближайшего человека
        if (obj->getType() == HUMAN && dist < minDist) {
            minDist = dist;
            closestVictim = obj;
        }

        // 1.2 и 1.4: Уклонение от своих (чтобы не слипались в одну точку)
        if (obj->getType() == ZOMBIE && dist < _safetyRadius) {
            targetVx -= (dx / dist) * (_maxSpeed * 0.3);
            targetVy -= (dy / dist) * (_maxSpeed * 0.3);

        }
        }
    }

    // Если нашли цель — движемся к ней
    if (closestVictim && !_isBusy) {
        double dx = closestVictim->getState().pos[0] - _curState.pos[0];
        double dy = closestVictim->getState().pos[1] - _curState.pos[1];
        double dist = std::sqrt(dx*dx + dy*dy);

        // Вектор скорости по заданию: v_max * (p2-p1)/|p2-p1|
        targetVx = (dx / dist) * _maxSpeed;
        targetVy = (dy / dist) * _maxSpeed;

        // Логика атаки
        if (dist < _biteRadius) {
            Human* h = qobject_cast<Human*>(closestVictim);
            if (h) {
                h->biteSlot(); // Превращаем в зомби
                _isBusy = true; // Зомби останавливается «пожевать»
                targetVx = 0;
                targetVy = 0;
            }
        }
    }

    _curState.vel[0] = targetVx;
    _curState.vel[1] = targetVy;

    // --- ПРИМЕНЕНИЕ ДВИЖЕНИЯ И ОТСКОК ---
    double newX = _curState.pos[0] + _curState.vel[0] * dt;
    double newY = _curState.pos[1] + _curState.vel[1] * dt;

    if (newX < 0 || newX > 500) {
        _curState.vel[0] *= -1;
        newX = (newX < 0) ? 0 : 500;
    }
    if (newY < 0 || newY > 500) {
        _curState.vel[1] *= -1;
        newY = (newY < 0) ? 0 : 500;
    }




    if (newX <= 0) { newX = 1; _curState.vel[0] *= -1; }
    if (newX >= 500) { newX = 499; _curState.vel[0] *= -1; }
    if (newY <= 0) { newY = 1; _curState.vel[1] *= -1; }
    if (newY >= 500) { newY = 499; _curState.vel[1] *= -1; }

    setPosition(newX, newY);
}
