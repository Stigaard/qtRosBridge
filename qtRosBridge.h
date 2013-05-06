#ifndef qtRosBridge_H
#define qtRosBridge_H

#include <QObject>
#include <QTimer>
#include <string>
#include <QString>
#include <QHostAddress>
#include "websocket.h"

class qtRosBridge : public QObject
{
Q_OBJECT
public:
    qtRosBridge(QHostAddress, quint32);
    virtual ~qtRosBridge();
    void subscribe(QString topic, QString type, qint32 throttle_rate = 0); 
private:
  websocket *ws;
private slots:
  void newData(void);
signals:
  void newMsg(QVariant msg);
};

#endif // qtRosBridge_H
