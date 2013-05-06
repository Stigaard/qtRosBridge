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


#include "websocket.h"
#include "websocket.moc"
#include <QDebug>
#include <vector>
#include <string>

websocket::websocket(QObject* parent)
{
  this->socket = new QTcpSocket(this);
  connect(this->socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(connectionStateChanged(QAbstractSocket::SocketState)));
  connect(this->socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectionError(QAbstractSocket::SocketError)));
  connect(this->socket, SIGNAL(readyRead()), this, SLOT(dataDumper()));
}

void websocket::dataDumper(void )
{
  this->rawRxBufLock.lock();
  quint64 N = this->socket->bytesAvailable();
  this->rawRxBuf.resize(this->rawRxBuf.size()+N);
  this->socket->read((char*)&(this->rawRxBuf[this->rawRxBuf.size()-N]), N);
  this->rawRxBufLock.unlock();
  this->decode();
}

void websocket::clearRxBuffer(void )
{
  this->rawRxBufLock.lock();
  this->rawRxBuf.clear();
  this->rawRxBufLock.unlock();
}


void websocket::decode(void )
{
  this->rawRxBufLock.lock();
  while (true) {
      wsheader_type ws;
      if (rawRxBuf.size() < 2) {this->rawRxBufLock.unlock(); return; /* Need at least 2 */ }
      const quint8 * data = (quint8 *) &rawRxBuf[0]; // peek, but don't consume
      ws.fin = (data[0] & 0x80) == 0x80;
      ws.opcode = (wsheader_type::opcode_type) (data[0] & 0x0f);
      ws.mask = (data[1] & 0x80) == 0x80;
      ws.N0 = (data[1] & 0x7f);
      ws.header_size = 2 + (ws.N0 == 126? 2 : 0) + (ws.N0 == 127? 6 : 0) + (ws.mask? 4 : 0);
      if (rawRxBuf.size() < ws.header_size) {this->rawRxBufLock.unlock();  return; /* Need: ws.header_size - rxbuf.size() */ }
      int i;
      if (ws.N0 < 126) {
	  ws.N = ws.N0;
	  i = 2;
      }
      else if (ws.N0 == 126) {
	  ws.N = 0;
	  ws.N |= ((quint64) data[2]) << 8;
	  ws.N |= ((quint64) data[3]) << 0;
	  i = 4;
      }
      else if (ws.N0 == 127) {
	  ws.N = 0;
	  ws.N |= ((quint64) data[2]) << 56;
	  ws.N |= ((quint64) data[3]) << 48;
	  ws.N |= ((quint64) data[4]) << 40;
	  ws.N |= ((quint64) data[5]) << 32;
	  ws.N |= ((quint64) data[6]) << 24;
	  ws.N |= ((quint64) data[7]) << 16;
	  ws.N |= ((quint64) data[8]) << 8;
	  ws.N |= ((quint64) data[9]) << 0;
	  i = 10;
      }
      if (ws.mask) {
	  ws.masking_key[0] = ((quint8) data[i+0]) << 0;
	  ws.masking_key[1] = ((quint8) data[i+1]) << 0;
	  ws.masking_key[2] = ((quint8) data[i+2]) << 0;
	  ws.masking_key[3] = ((quint8) data[i+3]) << 0;
      }
      else {
	  ws.masking_key[0] = 0;
	  ws.masking_key[1] = 0;
	  ws.masking_key[2] = 0;
	  ws.masking_key[3] = 0;
      }
      if (rawRxBuf.size() < ws.header_size+ws.N) {this->rawRxBufLock.unlock();   return; /* Need: ws.header_size+ws.N - rxbuf.size() */ }

      // We got a whole message, now do something with it:
      if (false) { }
      else if (ws.opcode == wsheader_type::TEXT_FRAME && ws.fin) {
	  if (ws.mask) { for (size_t i = 0; i != ws.N; ++i) { rawRxBuf[i+ws.header_size] ^= ws.masking_key[i&0x3]; } }
	  std::string data(rawRxBuf.begin()+ws.header_size, rawRxBuf.begin()+ws.header_size+ws.N);
	  rxBufLock.lock();
	  rxBuf.insert(rxBuf.end(), data.begin(), data.end());
	  //callable((const std::string) data);
	  rxBufLock.unlock();
	  emit(bytesAvailabe());
      }
//      else if (ws.opcode == wsheader_type::PING) { }
//      else if (ws.opcode == wsheader_type::PONG) { }
//      else if (ws.opcode == wsheader_type::CLOSE) { close(); }
      else {this->rawRxBuf.erase(this->rawRxBuf.begin());continue;}

      rawRxBuf.erase(rawRxBuf.begin(), rawRxBuf.begin() + ws.header_size+ws.N);
  }
  rawRxBufLock.unlock();
}

void websocket::connectionStateChanged(QAbstractSocket::SocketState state)
{
  qDebug() << "Connection state change to: " << state;
}

void websocket::connectionError(QAbstractSocket::SocketError err)
{
  qDebug() << "Connection error: " << err;
}


void websocket::connectToHost(const QHostAddress& address, quint16 port)
{
  this->address = address;
  this->port = port;
  qDebug() << "Connecting to host";
  this->socket->connectToHost(address, port, ReadWrite);
  qDebug() << "Waiting for connection";
  this->socket->waitForConnected();
  qDebug() << "Switching to websocket";
  this->switchHttpToWebsocket();
  qDebug() << "waiting for bytes written";
  this->socket->flush();
  this->socket->waitForBytesWritten();
  qDebug() << "bytes written";
  this->socket->flush();
  this->clearRxBuffer();
//  qDebug() << "Read raw:" << this->socket->read(32);
}

void websocket::switchHttpToWebsocket(void )
{ //TODO: Update to dynamic assignment of address and port
  
  this->socket->write("GET / HTTP/1.1\r\n");
  this->socket->write("Host: 127.0.0.1:9090\r\n");
  this->socket->write("Upgrade: websocket\r\n");
  this->socket->write("Connection: Upgrade\r\n");
  this->socket->write("Sec-WebSocket-Key: uRovscZjNol/umbTt5uKmw==\r\n");
  this->socket->write("Origin: ws://127.0.0.1:9090/\r\n");
  this->socket->write("Sec-WebSocket-Version: 13\r\n\r\n");
}

qint64 websocket::writeData(const char* data, qint64 len)
{
  std::vector<quint8> header, masking_key;
  std::vector<quint8> txbuf;
  masking_key.assign(4,0);
  std::string message;
  message.assign(len, 0);
  for(int i=0;i<len;i++)
    message[i] = data[i];
  
  header.assign(2 + (message.size() >= 126 ? 2 : 0) + (message.size() >= 65536 ? 6 : 0), 0);
  header[0] = 0x80 | wsheader_type::TEXT_FRAME;
    if (false) { }
    else if (message.size() < 126) {
	header[1] = message.size()+128;
    }
    else if (message.size() < 65536) {
	header[1] = 126+128;
	header[2] = (message.size() >> 8) & 0xff;
	header[3] = (message.size() >> 0) & 0xff;
    }
    else { // TODO: run coverage testing here
	header[1] = 127+128;
	header[2] = (message.size() >> 56) & 0xff;
	header[3] = (message.size() >> 48) & 0xff;
	header[4] = (message.size() >> 40) & 0xff;
	header[5] = (message.size() >> 32) & 0xff;
	header[6] = (message.size() >> 24) & 0xff;
	header[7] = (message.size() >> 16) & 0xff;
	header[8] = (message.size() >>  8) & 0xff;
	header[9] = (message.size() >>  0) & 0xff;
    }
    txbuf.insert(txbuf.end(), header.begin(), header.end());
    txbuf.insert(txbuf.end(), masking_key.begin(), masking_key.end());
    txbuf.insert(txbuf.end(), message.begin(), message.end());
    QByteArray buf;
    for(int i=0;i<txbuf.size();i++)
      buf.append(txbuf[i]);
    this->socket->write(buf);
}


qint64 websocket::writeData(const char* data)
{
  return this->writeData(data, qstrlen(data));
}

qint64 websocket::readData(char* data, qint64 maxlen)
{
  rxBufLock.lock();
  int N = rxBuf.size();
  maxlen--;
  if(N>maxlen)
    N = maxlen;
  if(N>0)
  {
    for(int i=0;i<N;i++)
    {
      data[i] = rxBuf[i];
    }
    rxBuf.erase(rxBuf.begin(), rxBuf.begin()+N);
    rxBufLock.unlock();
    data[N] = 0;
    return N;
  }
  rxBufLock.unlock();
  data[N] = 0;
  return 0;
}


void websocket::bytesAvailableVerifier(void )
{
  emit(bytesAvailabe());
}

