/*
    Copyright (c) 2013, Morten S. Laursen <email>
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the <organization> nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY Morten S. Laursen <email> ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL Morten S. Laursen <email> BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef QWEBSOCKET_H
#define QWEBSOCKET_H
#include <QIODevice>
#include <QTcpSocket>
#include <QHostAddress>
#include <vector>
#include <QMutex>

class websocket : public QIODevice
{
  Q_OBJECT
public:
    explicit websocket(QObject* parent = 0);
    void connectToHost(const QHostAddress& address, quint16 port);
    qint64 writeData(const char* data);
    qint64 readData(char* data, qint64 maxlen);
    qint64 writeData(const char* data, qint64 len);
    void clearRxBuffer(void);
private:
  void switchHttpToWebsocket(void);
  QHostAddress address;
  quint16 port;
  QTcpSocket * socket;
  void decode(void);
  struct wsheader_type {
      qint32 header_size;
      bool fin;
      bool mask;
      enum opcode_type {
	  CONTINUATION = 0x0,
	  TEXT_FRAME = 0x1,
	  BINARY_FRAME = 0x2,
	  CLOSE = 8,
	  PING = 9,
	  PONG = 0xa,
      } opcode;
      int N0;
      quint64 N;
      quint8 masking_key[4];
  };
  std::vector<quint8> rawRxBuf;
  std::vector<quint8> rxBuf;
  QMutex rawRxBufLock;
  QMutex rxBufLock;
private slots:
  void bytesAvailableVerifier(void);
  void connectionStateChanged(QAbstractSocket::SocketState);
  void connectionError(QAbstractSocket::SocketError);
  void dataDumper(void);
signals:
  void bytesAvailabe(void);
};

#endif // QWEBSOCKET_H
