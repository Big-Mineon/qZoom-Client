#include "sessionhandler.h"

SessionHandler::SessionHandler(ServerTcpQueries* _mServerTcpQueries, UserHandler* _user,
                               ImageHandler* imageHandler,
                               Settings* settings, int bufferSize,
                               QObject *parent) : QObject(parent)
{
    mServerTcpQueries = _mServerTcpQueries;
    mUser = _user;
    setDefaultRoomID();
    mIpAddress = "Ipaddress";
    mSettings = settings;
    mBufferSize = bufferSize;
    mPortNumberTCP = settings->getTcpPort();
    mPortNumberUDP = settings->getUdpPort();
    mImageHandler = imageHandler;
    mSessionIsActive = false;

    mAddress = (mSettings->getServerIpAddress() == "Localhost") ?
                QHostAddress::LocalHost : QHostAddress(mSettings->getServerIpAddress());

}

bool SessionHandler::enableScreenShare()
{
    if(mOutputStreamHandler->checkVideoEnabled())
    {
        mOutputStreamHandler->disableVideo();
    }
    return mOutputStreamHandler->enableVideo(true) >= 0;
}

void SessionHandler::kickParticipant(const int &index) const
{
    QString streamId = mInputStreamHandler->getStreamIdFromIndex(index);
    if (streamId.size() > 0)
    {
        mTcpSocketHandler->sendKickParticipantSignal(streamId);
    }
}

bool SessionHandler::isHost() const
{
    return (mRoomHostUsername == mUser->getUsername() && !mUser->isGuest());
}

bool SessionHandler::enableVideo()
{
    if(mOutputStreamHandler->checkVideoEnabled())
    {
        mOutputStreamHandler->disableVideo();

    }
    return mOutputStreamHandler->enableVideo() >= 0;
}

void SessionHandler::disableVideo()
{
    mOutputStreamHandler->disableVideo();
}

bool SessionHandler::enableAudio()
{
    mImageHandler->setPeerAudioIsDisabled(std::numeric_limits<uint8_t>::max(), false);
    return mOutputStreamHandler->enableAudio() >= 0;
}

void SessionHandler::disableAudio()
{
    mOutputStreamHandler->disableAudio();
    mImageHandler->setPeerAudioIsDisabled(std::numeric_limits<uint8_t>::max(), true);
}

int SessionHandler::initOtherStuff()
{
    const QString streamId = isGuest() ? mUser->getGuestStreamId() : mUser->getStreamId();
    const QString roomId = getRoomId();
    const QString displayName = mSettings->getDisplayName();
    mPortNumberUDP = mSettings->getUdpPort();
    mPortNumberTCP = mSettings->getTcpPort();
    mAddress = (mSettings->getServerIpAddress() == "Localhost") ? QHostAddress::LocalHost : QHostAddress(mSettings->getServerIpAddress());
    mSessionIsActive = true;
    mInputStreamHandler = new InputStreamHandler(mImageHandler, mBufferSize, mAddress);
    mUdpSocketHandler = new UdpSocketHandler(mBufferSize, mPortNumberUDP, mInputStreamHandler, streamId, roomId, mAddress);
    mTcpSocketHandler = new TcpSocketHandler(mInputStreamHandler, streamId, roomId, displayName, mAddress, mPortNumberTCP);
    connect(mTcpSocketHandler, &TcpSocketHandler::sendDummyDatagram, mUdpSocketHandler, &UdpSocketHandler::openPortHack);

    mOutputStreamHandler = new OutputStreamHandler(mImageHandler, mUdpSocketHandler, mBufferSize, mSettings, mTcpSocketHandler);

    if(mTcpSocketHandler->init() < 0)
    {
        return -1;
    }

    mOutputStreamHandler->init();
    return 0;
}


void SessionHandler::deleteSocketHandlers()
{
    qDebug() << Q_FUNC_INFO;
    delete mUdpSocketHandler;
    delete mTcpSocketHandler;
}

void SessionHandler::deleteStreams()
{
    delete mInputStreamHandler;
    delete mOutputStreamHandler;
}

void SessionHandler::closeSocketHandlers()
{
    mUdpSocketHandler->closeSocket();
    mTcpSocketHandler->close();
}

void SessionHandler::closeStreams()
{
    //This will clear all the vectors containing objects connected to each person in the room
    mInputStreamHandler->close();
    //This will close the output streams
    mOutputStreamHandler->close();
}

QVariantList SessionHandler::getAudioInputDevices() const
{
    return AudioHandler::getAudioInputDevices();
}

bool SessionHandler::joinSession(const QString& roomId, const QString& roomPassword)
{
    qDebug() << "RoomId: " << roomId;
    QVariantList vars;
    vars.append(roomId);
    vars.append(roomPassword);
    const QVariantList response = mServerTcpQueries->serverQuery(0, vars);
    if(response.size() > 0)
    {
        if(response[0].toInt() == -1)
        {
            errorHandler->giveErrorDialog("Could not connect to server");
            return false;
        }

        mRoomId = response[0].toString();
        mRoomPassword = response[1].toString();
        qDebug() << "mRoomPassword: " << mRoomPassword;
        mRoomHostUsername = response[2].toString();
        addUser();
        mSettings->setLastRoomId(mRoomId);
        mSettings->setLastRoomPassword(mRoomPassword);
        mSettings->saveSettings();
        const uint8_t userIndex = std::numeric_limits<uint8_t>::max();
        mImageHandler->addPeer(userIndex, mSettings->getDisplayName());

        if(initOtherStuff() < 0)
        {
            leaveSession();
            errorHandler->giveErrorDialog("An unknown error occured when initializing stream");
            return false;
        }
        return true;
    }
    return false;
}

QString SessionHandler::getRoomId() const
{
    return mRoomId;
}

QString SessionHandler::getRoomPassword() const
{
    return mRoomPassword;
}

void SessionHandler::addUser()
{
    QVariantList vars;
    vars.append(mRoomId);

    qDebug() << "User is guest: " << mUser->isGuest();
    if (!mUser->isGuest())
    {
        vars.append(QString::number(mUser->getUserId()));
        const int numberOfRowsAffected = mServerTcpQueries->serverQuery(1, vars)[0].toInt();
        if(numberOfRowsAffected <= 0)
        {
            qDebug() << "Failed Query" << Q_FUNC_INFO;
        }
    }
    else
    {
        if (addGuestUserToDatabase())
        {
            vars.append(QString::number(mUser->getGuestId()));
            const int numberOfRowsAffected = mServerTcpQueries->serverQuery(1, vars)[0].toInt();
            if(numberOfRowsAffected <= 0)
            {
                qDebug() << "Failed Query" << Q_FUNC_INFO;
            }
        }
    }
}

void SessionHandler::kickYourself()
{
    mSessionIsActive = false;
    closeSocketHandlers();
    closeStreams();
    errorHandler->giveKickedErrorDialog();
}

void SessionHandler::deleteStreamsAndSockets()
{
    deleteSocketHandlers();
    deleteStreams();
    mImageHandler->removeAllPeers();
}

bool SessionHandler::leaveSession()
{
    mSessionIsActive = false;
    closeSocketHandlers();
    closeStreams();
    deleteSocketHandlers();
    deleteStreams();
    mImageHandler->removeAllPeers();
    return true;
}

bool SessionHandler::createSession(const QString& roomId, const QString& roomPassword)
{
    if (!mUser->isGuest())
    {
        if (!mUser->hasRoom())
        {
            QVariantList vars;
            vars.append(roomPassword);
            vars.append(QString::number(mUser->getUserId()));
            vars.append(roomId);
            int numberOfRowsAffected = mServerTcpQueries->serverQuery(2, vars)[0].toInt();
            if(numberOfRowsAffected <= 0)
            {
                qDebug() << "Failed Query" << Q_FUNC_INFO;
                return false;
            }
            return true;
        }
        else
        {
            qDebug() << "User already has a room";
        }
    }
    return false;
}

bool SessionHandler::isGuest() const
{
    return mUser->isGuest();
}

QString SessionHandler::getRoomHostUsername() const
{
    return mRoomHostUsername;
}

bool SessionHandler::addGuestUserToDatabase()
{
    qDebug() << mUser->getGuestName();
    QVariantList vars;
    vars.append(mUser->getGuestName());
    int numberOfRowsAffected = mServerTcpQueries->serverQuery(3, vars)[0].toInt();
    if(numberOfRowsAffected <= 0)
    {
        qDebug() << "Failed Query" << Q_FUNC_INFO;
        return false;
    }
    qDebug() << "Added guest :" << mUser->getGuestName() << "to the database";
    return true;
}

bool SessionHandler::getSessionIsActive() const
{
    return mSessionIsActive;
}

void SessionHandler::setDefaultRoomID()
{
    mRoomId = "Debug";
}

void SessionHandler::updateDisplayName()
{
    qDebug() << "SessionHandler will updateDisplayName";
    if (mSessionIsActive)
    {
        qDebug() << "Session is active";
        mImageHandler->updatePeerDisplayName(std::numeric_limits<uint8_t>::max(), mSettings->getDisplayName());
        mTcpSocketHandler->updateDisplayName(mSettings->getDisplayName());
        mTcpSocketHandler->sendChangedDisplayNameSignal();
    }
}

bool SessionHandler::checkVideoEnabled() const
{
    return mOutputStreamHandler->checkVideoEnabled();
}

bool SessionHandler::checkAudioEnabled() const
{
    return mOutputStreamHandler->checkAudioEnabled();
}
