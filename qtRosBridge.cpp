#include "qtRosBridge.h"


#include <iostream>
#include <assert.h>
#include <QDebug>
#include <QString>
#include <QVariant>
#include <qjson/parser.h>

qtRosBridge::qtRosBridge(QHostAddress adr, quint32 port)
{
  ws = new websocket(this);
  connect(ws, SIGNAL(bytesAvailabe()), this, SLOT(newData()));
  ws->connectToHost(adr, port);
}

qtRosBridge::~qtRosBridge()
{
}

void qtRosBridge::newData(void )
{
  quint8 data[4096];
  this->ws->readData((char*)data, 4096);
  QJson::Parser parser;
  bool ok;
  QVariant result = parser.parse((char*)data, &ok);
  if(ok)
    emit(newMsg(result));
}

void qtRosBridge::subscribe(QString topic, QString type, qint32 throttle_rate)
{
  QString msg = "{\"topic\": \"" + topic + "\", \"type\": \"" + type + "\", \"op\": \"subscribe\"}\r\n";
  ws->writeData(msg.toUtf8());
}

#include "qtRosBridge.moc"
