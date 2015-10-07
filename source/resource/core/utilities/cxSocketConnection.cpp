/*=========================================================================
This file is part of CustusX, an Image Guided Therapy Application.

Copyright (c) 2008-2014, SINTEF Department of Medical Technology
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=========================================================================*/

#include "cxSocketConnection.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkInterface>
#include <QThread>

#include "cxSocket.h"
#include "cxLogger.h"

SNW_DEFINE_ENUM_STRING_CONVERTERS_BEGIN(cx, CX_SOCKETCONNECTION_STATE, scsCOUNT)
{
	"inactive",
	"connected",
	"listening",
	"connecting"
}
SNW_DEFINE_ENUM_STRING_CONVERTERS_END(cx, CX_SOCKETCONNECTION_STATE, scsCOUNT)



namespace cx
{


	bool SocketConnection::ConnectionInfo::operator==(const ConnectionInfo& rhs) const
	{
		return (this->role == rhs.role)
				&& (this->protocol == rhs.protocol)
				&& (this->host == rhs.host)
				&& (this->port == rhs.port);
	}

	bool SocketConnection::ConnectionInfo::isServer() const { return role.toLower()=="server"; }
	bool SocketConnection::ConnectionInfo::isClient() const { return !this->isServer(); }
	QString SocketConnection::ConnectionInfo::getDescription() const
	{
		QString postfix;
		QString name = host;
		if (isServer())
			name = "listen";
		if (!protocol.isEmpty())
			postfix = QString("[%1]").arg(protocol);
		return QString("%1:%2%3").arg(name).arg(port).arg(postfix);
	}

////---------------------------------------------------------
////---------------------------------------------------------
////---------------------------------------------------------


SocketConnection::SocketConnection(QObject *parent) :
	QObject(parent)
{
	mCurrentState = scsINACTIVE;
	qRegisterMetaType<boost::function<void()> >("boost::function<void()>");
	qRegisterMetaType<CX_SOCKETCONNECTION_STATE>("CX_SOCKETCONNECTION_STATE");

	mNextConnectionInfo.host = "localhost";
	mNextConnectionInfo.port = 18944;
	mNextConnectionInfo.role = "client";

	mSocket = new QTcpSocket(this);
	connect(mSocket, &QTcpSocket::connected, this, &SocketConnection::internalConnected);
	connect(mSocket, &QTcpSocket::disconnected, this, &SocketConnection::internalDisconnected);
	connect(mSocket, &QTcpSocket::readyRead, this, &SocketConnection::internalDataAvailable);

	//see http://stackoverflow.com/questions/26062397/qt-connect-function-signal-disambiguation-using-lambdas
	void (QTcpSocket::* errorOverloaded)(QAbstractSocket::SocketError) = &QTcpSocket::error;
	connect(mSocket, errorOverloaded, this, &SocketConnection::error);
	connect(mSocket, errorOverloaded, this, &SocketConnection::internalError);
}

SocketConnection::ConnectionInfo SocketConnection::getConnectionInfo()
{
	QMutexLocker locker(&mNextConnectionInfoMutex);
	return mNextConnectionInfo;
}

void SocketConnection::setConnectionInfo(ConnectionInfo info)
{
	QMutexLocker locker(&mNextConnectionInfoMutex);
	if (info==mNextConnectionInfo)
		return;
	mNextConnectionInfo = info;
	locker.unlock();

	emit connectionInfoChanged();
}

void SocketConnection::stateChange(CX_SOCKETCONNECTION_STATE newState)
{
	if (mCurrentState==newState)
		return;

	if (newState==scsCONNECTED)
		emit connected();
	else if (mCurrentState==scsCONNECTED)
		emit disconnected();

	mCurrentState = newState;

	emit stateChanged(mCurrentState);
}

CX_SOCKETCONNECTION_STATE SocketConnection::getState()
{
	return mCurrentState;
}

void SocketConnection::requestConnect()
{
	assertRunningInObjectThread();

	ConnectionInfo info = this->getConnectionInfo();
	mConnector = this->createConnector(info);
	this->setProtocol(info.protocol);
	mConnector->activate();
}

SocketConnectorPtr SocketConnection::createConnector(ConnectionInfo info)
{
	SocketConnectorPtr retval;

	if (info.isClient())
		retval.reset(new SocketClientConnector(info, mSocket));
	else
		retval.reset(new SocketServerConnector(info, mSocket));

	connect(retval.get(), &SocketConnector::stateChanged, this, &SocketConnection::stateChange);
	return retval;
}

void SocketConnection::requestDisconnect()
{
	assertRunningInObjectThread();

	if (mConnector)
	{
		mConnector->deactivate();
		mConnector.reset();
	}
	mSocket->close();
}

bool SocketConnection::sendData(const char *data, qint64 maxSize)
{
    if(!this->socketIsConnected())
        return false;
    qint64 writtenBytes = mSocket->write(data, maxSize);
    if(writtenBytes != maxSize)
        return false;
    return true;
}

void SocketConnection::internalConnected()
{
//	CX_LOG_SUCCESS() << "Connected to "  << mCurrentConnectionInfo.getDescription();
	this->stateChange(this->computeState());
}

void SocketConnection::internalDisconnected()
{
//    CX_LOG_SUCCESS() << "Disconnected";
	this->stateChange(this->computeState());
}

void SocketConnection::internalError(QAbstractSocket::SocketError socketError)
{
	CX_LOG_ERROR() << QString("[%1] Socket error [code=%2]: %3")
					  .arg(mSocket->peerName())
					  .arg(QString::number(socketError))
					  .arg(mSocket->errorString());

	this->stateChange(this->computeState());
}

//void SocketConnection::internalDataAvailable()
//{
//    if(!this->socketIsConnected())
//        return;

//	// How many bytes?
//	qint64 maxAvailableBytes = mSocket->bytesAvailable();

//    char* inMessage = new char [maxAvailableBytes];
//    if(!this->socketReceive(inMessage, maxAvailableBytes))
//        return;

//    CX_LOG_INFO() << "SocketConnection incoming message: " << QString(inMessage);

//    //TODO: Do something with received message
//}

bool SocketConnection::socketIsConnected()
{
	return mSocket->state() == QAbstractSocket::ConnectedState;
}

bool SocketConnection::enoughBytesAvailableOnSocket(int bytes) const
{
    bool enoughBytes = mSocket->bytesAvailable() >= bytes;
    if(!enoughBytes)
        CX_LOG_DEBUG() << "Want " << bytes << " but only "<< mSocket->bytesAvailable() << " are available on the socket atm.";
    return enoughBytes;
}

bool SocketConnection::socketReceive(void *packPointer, int packSize) const
{
    if(!this->enoughBytesAvailableOnSocket(packSize))
        return false;

    char* charPointer = reinterpret_cast<char*>(packPointer);
    int r = mSocket->read(charPointer, packSize);
    if(r <= 0)
    {
        CX_LOG_ERROR() << "Error when receiving data from socket.";
        return false;
    }
    return true;
}

CX_SOCKETCONNECTION_STATE SocketConnection::computeState()
{
	if (mConnector)
		return mConnector->getState();
	return scsINACTIVE;
}


} //namespace cx
