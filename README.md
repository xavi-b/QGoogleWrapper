# QGoogleWrapper

Qt wrapper for Google APIs using OAuth 2.0 for TV and Limited-Input Device Applications

https://developers.google.com/identity/protocols/OAuth2ForDevices

## Usage

```cpp
QGoogleWrapper* wrapper = new QGoogleWrapper("/path/to/google/client.json");

connect(wrapper, &QGoogleWrapper::pendingVerification, [&](QString const& verificationUrl, QString const& userCode)
{
    qDebug() << "Verification URL and user code to display" << verificationUrl << userCode;
});
connect(wrapper, &QGoogleWrapper::pollingTimedOut, [&]()
{
    // maybe ask for device code again
});
connect(wrapper, &QGoogleWrapper::deviceCodeRequestError, [&](QString const& error)
{
    qDebug() << error;
    // error handling
});
connect(wrapper, &QGoogleWrapper::pollingRequestError, [&](QString const& error)
{
    qDebug() << error;
    // error handling
});
connect(wrapper, &QGoogleWrapper::accessTokenRequestError, [&](QString const& error)
{
    qDebug() << error;
    // error handling
});

// API example
connect(wrapper, &QGoogleWrapper::authenticated, [&]()
{
    // display authentication success information

    // get 1000 first contacts from Google Contacts API
    QNetworkRequest request;
    request.setUrl(QUrl("https://www.google.com/m8/feeds/contacts/default/full?max-results=1000"));
    request.setRawHeader("GData-Version", "3.0");
    auto reply = this->wrapper->apiCall(request);
    connect(reply, &QNetworkReply::finished, this, [=]()
    {
        QByteArray bytes = reply->readAll();
        qDebug() << bytes;
        reply->deleteLater();
    });
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this, [=](QNetworkReply::NetworkError error)
    {
        qDebug() << reply->errorString();
        // error handling
    });
});

// asking for a device code for the specified scope
wrapper->askDeviceCode("https://www.google.com/m8/feeds");
```

## License

LGPL-3.0

See LICENSE file for more informations.
