#ifndef QGOOGLEWRAPPER_H
#define QGOOGLEWRAPPER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QFile>

class QGoogleWrapper : public QObject
{
    Q_OBJECT
private:
    QNetworkAccessManager* manager;
    QTimer* poller;
    QNetworkReply* tokenReply = nullptr;
    QString clientId;
    QString clientSecret;
    QString deviceCode;
    QString accessToken;
    QString refreshToken;

public:
    QGoogleWrapper(QString const& jsonFilename, QObject* parent = nullptr);

    void askDeviceCode(QString const& scope);
    void askAccessToken();
    void startPolling(qint64 expiresIn, qint64 interval);
    void poll();
    QNetworkReply* apiCall(QNetworkRequest const& request);

    QString getRefreshToken() const { return this->refreshToken; }
    void setRefreshToken(QString const& refreshToken) { this->refreshToken = refreshToken; }

signals:
    void pendingVerification(QString const& verificationUrl, QString const& userCode);
    void pollingTimedOut();
    void authenticated();
    void deviceCodeRequestError(QString const& error);
    void pollingRequestError(QString const& error);
    void accessTokenRequestError(QString const& error);
    void log(QString const& log);

};

#endif // GOOGLEWRAPPER_H
