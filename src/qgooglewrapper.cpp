#include "qgooglewrapper.h"

QGoogleWrapper::QGoogleWrapper(QString const& jsonFilename, QObject *parent) : QObject(parent)
{
    this->manager = new QNetworkAccessManager(this);
    this->poller = new QTimer(this);

    QFile file(jsonFilename);
    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        const auto json = QJsonDocument::fromJson(file.readAll()).object()["installed"].toObject();
        this->clientId = json["client_id"].toString();
        this->clientSecret = json["client_secret"].toString();
    }
}

void QGoogleWrapper::askDeviceCode(QString const& scope)
{
    QNetworkRequest request;
    request.setUrl(QUrl("https://oauth2.googleapis.com/device/code"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QByteArray data;
    data.append("client_id=");
    data.append(this->clientId);
    data.append("&");
    data.append("scope=");
    data.append(scope);

    QNetworkReply* reply = this->manager->post(request, data);
    connect(reply, &QNetworkReply::finished, this, [=]()
    {
        auto bytes = reply->readAll();
        emit log("deviceCodeReply " + bytes);

        if(bytes.isEmpty())
        {
            emit deviceCodeRequestError("empty_reply");
            return;
        }

        auto map = QJsonDocument::fromJson(bytes).object().toVariantMap();

        if(map.find("error_code") != map.end())
        {
            emit deviceCodeRequestError(map.find("error_code").value().toString());
        }
        else if(map.find("verification_url") != map.end()
                && map.find("user_code") != map.end()
                && map.find("expires_in") != map.end()
                && map.find("interval") != map.end()
                && map.find("device_code") != map.end())
        {
            this->deviceCode = map.find("device_code").value().toString();

            emit pendingVerification(map.find("verification_url").value().toString(),
                                     map.find("user_code").value().toString());

            this->startPolling(map.find("expires_in").value().toDouble()*1000, map.find("interval").value().toDouble()*1000);
        }
        else
        {
            emit deviceCodeRequestError("incomplete_reply");
        }
        reply->deleteLater();
    });
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this, [=](QNetworkReply::NetworkError error)
    {
        emit log("deviceCodeReply " + reply->errorString());
    });
}

void QGoogleWrapper::askAccessToken()
{
    QNetworkRequest request;
    request.setUrl(QUrl("https://oauth2.googleapis.com/token"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QByteArray data;
    data.append("refresh_token=");
    data.append(this->refreshToken);
    data.append("&");
    data.append("client_id=");
    data.append(this->clientId);
    data.append("&");
    data.append("client_secret=");
    data.append(this->clientSecret);
    data.append("&");
    data.append("grant_type=");
    data.append("refresh_token");

    QNetworkReply* reply = this->manager->post(request, data);
    connect(reply, &QNetworkReply::finished, this, [=]()
    {
        auto bytes = reply->readAll();
        emit log("accessTokenReply " + bytes);

        if(bytes.isEmpty())
        {
            emit accessTokenRequestError("empty_reply");
            return;
        }

        auto map = QJsonDocument::fromJson(bytes).object().toVariantMap();

        if(map.find("error_code") != map.end())
        {
            emit accessTokenRequestError(map.find("error_code").value().toString());
        }
        else if(map.find("access_token") != map.end()
                && map.find("expires_in") != map.end()
                && map.find("token_type") != map.end())
        {
            this->accessToken = map.find("access_token").value().toString();
            this->refreshToken = map.find("refresh_token").value().toString();
            emit authenticated();
        }
        else
        {
            emit accessTokenRequestError("incomplete_reply");
        }
        reply->deleteLater();
    });
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this, [=](QNetworkReply::NetworkError error)
    {
        emit log("accessTokenReply " + reply->errorString());
    });
}

void QGoogleWrapper::startPolling(qint64 expiresIn, qint64 interval)
{
    disconnect(this->poller, 0, 0, 0);
    this->poller->start(interval);
    connect(this->poller, &QTimer::timeout, this, [=]()
    {
        static qint64 totalTime = 0;
        ++totalTime;
        if(totalTime > expiresIn)
        {
            this->poller->stop();
            emit pollingTimedOut();
        }
        else
        {
            this->poll();
        }
    });
}

void QGoogleWrapper::poll()
{
    if(this->tokenReply != nullptr)
        this->tokenReply->abort();

    QNetworkRequest request;
    request.setUrl(QUrl("https://oauth2.googleapis.com/token"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QByteArray data;
    data.append("client_id=");
    data.append(this->clientId);
    data.append("&");
    data.append("client_secret=");
    data.append(this->clientSecret);
    data.append("&");
    data.append("device_code=");
    data.append(this->deviceCode);
    data.append("&");
    data.append("grant_type=");
    data.append(QUrl::toPercentEncoding("urn:ietf:params:oauth:grant-type:device_code"));

    this->tokenReply = manager->post(request, data);
    connect(this->tokenReply, &QNetworkReply::finished, this, [&]()
    {
        if(this->tokenReply == nullptr)
            return;

        if(this->tokenReply->error() == QNetworkReply::OperationCanceledError)
            return;

        auto bytes = this->tokenReply->readAll();
        emit log("tokenReply " + bytes);

        if(bytes.isEmpty())
        {
            emit pollingRequestError("empty_reply");
            return;
        }

        auto map = QJsonDocument::fromJson(bytes).object().toVariantMap();

        if(map.find("error") != map.end())
        {
            emit pollingRequestError(map.find("error").value().toString());
        }
        else if(map.find("access_token") != map.end()
                && map.find("expires_in") != map.end()
                && map.find("token_type") != map.end()
                && map.find("refresh_token") != map.end())
        {
            this->poller->stop();
            this->accessToken = map.find("access_token").value().toString();
            this->refreshToken = map.find("refresh_token").value().toString();
            emit authenticated();
        }
        else
        {
            emit pollingRequestError("incomplete_reply");
        }
        this->tokenReply->deleteLater();
        this->tokenReply = nullptr;
    });
    connect(this->tokenReply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this, [=](QNetworkReply::NetworkError error)
    {
        emit log("tokenReply " + this->tokenReply->errorString());
    });
}

QNetworkReply* QGoogleWrapper::apiCall(QNetworkRequest const& request)
{
    QNetworkRequest r(request);
    r.setRawHeader("Authorization", ("Bearer " + this->accessToken).toUtf8());
    return manager->get(r);
}
