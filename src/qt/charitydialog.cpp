#include "charitydialog.h"
#include "ui_charitydialog.h"
#include "walletmodel.h"
#include "addressbookpage.h"
#include "init.h"
#include "base58.h"
#include <QLineEdit>
#include <QFile>
#include <QtNetwork>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QClipboard>
#include <QDesktopWidget>

StakeForCharityDialog::StakeForCharityDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StakeForCharityDialog),
    model(0)
{
    ui->setupUi(this);
    loadCharities();

    ui->label_2->setFocus();
}
QString charitiesAddress[100];
QString charitiesAsk[100];
QString charitiesThanks[100];
QString charitiesImage[100];
QString charitiesURL[100];
//QString charitiesSPEROImage[100];
//QString charitiesSPEROURL[100];

StakeForCharityDialog::~StakeForCharityDialog()
{
    delete ui;
}

void StakeForCharityDialog::setModel(WalletModel *model)
{
    this->model = model;

    CBitcoinAddress strAddress;
    CBitcoinAddress strChangeAddress;
    int nPer;
    qint64 nMin;
    qint64 nMax;
    charitiesThanks[0]="Thank you for donating to the <a href='https://sperocoin.org/SPEROFoundation/' style='color: #1ab06c;'>SperoCoin Foundation, Inc.</a>";

    model->getStakeForCharity(nPer, strAddress, strChangeAddress, nMin, nMax);

    if (strAddress.IsValid() && nPer > 0 )
    {
        ui->charityAddressEdit->setText(strAddress.ToString().c_str());
        ui->charityPercentEdit->setText(QString::number(nPer));
        if (strChangeAddress.IsValid())
            ui->charityChangeAddressEdit->setText(strChangeAddress.ToString().c_str());
        if (nMin > 0  && nMin != MIN_TX_FEE)
            ui->charityMinEdit->setText(QString::number(nMin/COIN));
        if (nMax > 0 && nMax != MAX_MONEY)
            ui->charityMaxEdit->setText(QString::number(nMax/COIN));

        if (strAddress.ToString().c_str() == QString(FOUNDATION_POS) || strAddress.ToString().c_str() == QString(FOUNDATION_TEST_POS) )
        {
            ui->message->setStyleSheet("QLabel { color: #1ab06c; font-weight: 900; }");
            ui->message->setText(tr("Thank you for giving to The SperoCoin Foundation \n\n"));
            ui->comboBox->setCurrentIndex(1);
        }

        else
        {
            int i;
            for (i=0; i < 100; i++)
            {
                if ( QString::compare(charitiesAddress[i].toStdString().c_str(), strAddress.ToString().c_str(), Qt::CaseSensitive) == 0 )
                { if (fTestNet) qDebug() << charitiesAddress[i].toStdString().c_str() ;
                    ui->message->setStyleSheet("QLabel { color: #1ab06c; font-weight: 900; }");
                    ui->message->setText(charitiesThanks[i]);
                    ui->comboBox->setCurrentIndex(i+1);
                    break;
                }
            }
            if (i==100) // STS SPERO must be a 'manual' entered address
            {
                ui->message->setStyleSheet("QLabel { color: #1ab06c; font-weight: 900; }");
                ui->message->setText(tr("You are sending to manually entered address: ") + strAddress.ToString().c_str());
                ui->comboBox->setCurrentIndex(0);               
            }
        }


        if (pwalletMain->IsLocked())
        {
            ui->message->setStyleSheet("QLabel { color: red; font-weight: 900; }");
            ui->message->setText(ui->message->text() + "<br />" + "You must unlock your SperoCoin software <br />for Staking for Charity to proceed to: " + strAddress.ToString().c_str());
        }
    }
}

void StakeForCharityDialog::loadCharities()
{   
    if (fTestNet) qDebug() << "in loadCharities \n";
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::TlsV1_2);
    QNetworkRequest charitiesRequest;
    charitiesRequest.setSslConfiguration(config);
    charitiesRequest.setUrl(QUrl(QString("https://sperocoin.org/charities/charities.json")));
    charitiesRequest.setHeader(QNetworkRequest::ServerHeader, "application/json");
    charitiesRequest.setAttribute(
                QNetworkRequest::CacheLoadControlAttribute,
                QVariant(int(QNetworkRequest::AlwaysNetwork))
                );
    QNetworkAccessManager nam;
    QNetworkReply *reply = nam.get(charitiesRequest);
    while(!reply->isFinished())
    {
        qApp->processEvents();
    }

    if (reply->error() == QNetworkReply::NoError) {
        QString strReply = (QString)reply->readAll();
        int i;
        int n = ui->comboBox->count();
        // clear current combo box entires, leaving the first 2, "manual" and "SPERO foundation"
        for (i=2; i < n; i++ ) {
            ui->comboBox->removeItem(2);
            if (fTestNet) qDebug() << "removed " << i+1 << " of " << n << "\n";
        }

        QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8() );
        QJsonArray json_array = jsonResponse.array();
        i = 1;
        // load new combo box entires
        foreach (const QJsonValue &value, json_array) {
            QJsonObject json_obj = value.toObject();
            if (fTestNet) qDebug() << json_obj["charity"].toString();
            ui->comboBox->addItem(QString(json_obj["charity"].toString()));
            if (fTestNet) qDebug() << json_obj["SPEROaddress"].toString();
            charitiesAddress[i] = json_obj["SPEROaddress"].toString();
            charitiesThanks[i] = json_obj["thanksOnly"].toString();
            charitiesAsk[i] = json_obj["ask"].toString();
            ui->comboBox->setItemData(i+1, json_obj["ask"].toString().replace("<a href","<a style='color: #ffffff;' href") , Qt::ToolTipRole);
            if (fTestNet) qDebug() << json_obj["thanksOnly"].toString();
            charitiesImage[i] = json_obj["img"].toString();
            charitiesURL[i] = json_obj["url"].toString();
            //charitiesSPEROImage[i] = json_obj["SPEROimg"].toString();
            //charitiesSPEROURL[i] = json_obj["SPEROurl"].toString();
            i++;
        }
        ui->comboBox->setItemData(1, "Your donation will be used by the <a href='https://sperocoin.org/SPEROFoundation/' style='color: #ffffff;'>SperoCoin Foundation, Inc.</a> <br />under the guidance of the board and community.", Qt::ToolTipRole);
        ui->comboBox->setItemData(0, "You can donate to any SperoCoin address you wish.", Qt::ToolTipRole);
    } else {
        uiInterface.ThreadSafeMessageBox("HTTP Error: " + reply->errorString().toStdString(), "SperoCoin Dynamic Stake for Charity", CClientUIInterface::OK | CClientUIInterface::ICON_EXCLAMATION | CClientUIInterface::MODAL);
    }
    reply->deleteLater();
}

void StakeForCharityDialog::setAddress(const QString &address)
{
    ui->comboBox->setCurrentIndex(0);
    setAddress(address, ui->charityAddressEdit);
}

void StakeForCharityDialog::setAddress(const QString &address, QLineEdit *addrEdit)
{
    ui->comboBox->setCurrentIndex(0);
    addrEdit->setText(address);
    addrEdit->setFocus();
}

void StakeForCharityDialog::on_addressBookButton_clicked()
{
    if (model && model->getAddressTableModel())
    {
        AddressBookPage dlg(AddressBookPage::ForSending, AddressBookPage::SendingTab, this);
        dlg.setModel(model->getAddressTableModel());
        if (dlg.exec())
            setAddress(dlg.getReturnValue(), ui->charityAddressEdit);
    }
}

void StakeForCharityDialog::on_changeAddressBookButton_clicked()
{
    if (model && model->getAddressTableModel())
    {
        AddressBookPage dlg(AddressBookPage::ForSending, AddressBookPage::ReceivingTab, this);
        dlg.setModel(model->getAddressTableModel());
        if (dlg.exec())
            setAddress(dlg.getReturnValue(), ui->charityChangeAddressEdit);
    }
}

void StakeForCharityDialog::on_enableButton_clicked()
{
    if(model->getEncryptionStatus() == WalletModel::Locked)
    {
        ui->message->setStyleSheet("QLabel { color: red; font-weight: 900;}");
        ui->message->setText("Please unlock your SperoCoin software for sending (not staking only) <br /> for SperoCoin Staking for Charity to proceed.");
        return;
    }
    else if (fWalletUnlockStakingOnly)
    {
        ui->message->setStyleSheet("QLabel { color: red; font-weight: 900;}");
        ui->message->setText("Please unlock your SperoCoin software for spending <br /> (not staking only) for SperoCoin Staking for Charity to proceed.");
        return;
    }

    bool fValidConversion = false;
    qint64 nMinAmount = MIN_TX_FEE;
    qint64 nMaxAmount = MAX_MONEY;
    CBitcoinAddress changeAddress = "";

    CBitcoinAddress address = ui->charityAddressEdit->text().toStdString();
    if (!address.IsValid())
    {
        ui->message->setStyleSheet("QLabel { color: red; font-weight: 900; }");
        ui->message->setText(tr("The entered address:<br /> \"") + ui->charityAddressEdit->text() + tr("\"<br />is invalid.<br />Please check the address and try again <br />or select from the drop-down menu."));
        ui->charityAddressEdit->setFocus();
        return;
    }

    if (model->isMine(address))
    {
       ui->message->setStyleSheet("QLabel { color: red; font-weight: 900;}");
       ui->message->setText(tr("The entered address: <br /> ") + ui->charityAddressEdit->text() + tr(" <br />is your own. <br />Please check the address and try again <br />or select from the drop-down menu."));
       ui->charityAddressEdit->setFocus();
       return;
    }

    int nCharityPercent = ui->charityPercentEdit->text().toInt(&fValidConversion, 10);
    if (!fValidConversion || nCharityPercent > 100 || nCharityPercent <= 0)
    {
        ui->message->setStyleSheet("QLabel { color: red; font-weight: 900; }");
        ui->message->setText(tr("Please enter whole numbers, 1 through 100, for percentage")+ " \n\n\n");
        ui->charityPercentEdit->setFocus();
        ui->charityPercentEdit->setStyleSheet("border-color: #ffffff;");
        return;
    }

    if (!ui->charityMinEdit->text().isEmpty())
    {
        nMinAmount = ui->charityMinEdit->text().toDouble(&fValidConversion) * COIN;
        if(!fValidConversion || nMinAmount <= MIN_TX_FEE || nMinAmount >= MAX_MONEY  )
        {
            ui->message->setStyleSheet("QLabel { color: red; font-weight: 900; }");
            ui->message->setText(tr("Min Amount is too low, please re-enter.")+ " \n\n\n");
            ui->charityMinEdit->setFocus();
            return;
        }
    }

    if (!ui->charityMaxEdit->text().isEmpty())
    {
        nMaxAmount = ui->charityMaxEdit->text().toDouble(&fValidConversion) * COIN;
        if(!fValidConversion || nMaxAmount <= MIN_TX_FEE || nMaxAmount >= MAX_MONEY  )
        {
            ui->message->setStyleSheet("QLabel { color: red; font-weight: 900; }");
            ui->message->setText(tr("Max Amount is too high, please re-enter.")+ " \n\n\n");
            ui->charityMaxEdit->setFocus();
            return;
        }
    }

    if (nMinAmount >= nMaxAmount)
    {
        ui->message->setStyleSheet("QLabel { color: red; font-weight: 900;}");
        ui->message->setText(tr("Min Amount more than Max Amount, please re-enter.")+ " \n\n\n");
        ui->charityMinEdit->setFocus();
        return;
    }

    if (!ui->charityChangeAddressEdit->text().isEmpty())
    {
        changeAddress = ui->charityChangeAddressEdit->text().toStdString();
        if (!changeAddress.IsValid())
        {
            ui->message->setStyleSheet("QLabel { color: red; font-weight: 900;}");
            ui->message->setText("The entered change address:<br />\"" + ui->charityChangeAddressEdit->text() + "\" <br />is invalid. <br />Please check the address and try again.");
            ui->charityChangeAddressEdit->setFocus();
            return;
        }
        else if (!model->isMine(changeAddress))
        {
           ui->message->setStyleSheet("QLabel { color: red; font-weight: 900;}");
           ui->message->setText("The entered change address:<br />\"" + ui->charityChangeAddressEdit->text() + "\"<br />is not your own. <br />Please check the address and try again.");
           ui->charityChangeAddressEdit->setFocus();
           return;
        }
    }

    model->setStakeForCharity(true, nCharityPercent, address, changeAddress, nMinAmount, nMaxAmount);
    if(!fGlobalStakeForCharity)
         fGlobalStakeForCharity = true;
    ui->message->setStyleSheet("QLabel { color: #1ab06c; font-weight: 900;}");
    ui->message->setText("SperoCoin Staking For Charity enabled to:<br /> " + QString(address.ToString().c_str()) + " at a rate of " + QString::number(nCharityPercent) + "%");
    if (ui->comboBox->currentIndex() > 0) ui->message->setText(ui->message->text() + "<br />" + charitiesThanks[ui->comboBox->currentIndex()-1].replace("<a href","<a style='color: #1ab06c;' href")) ;
    ui->comboBox->update();
    return;
}

void StakeForCharityDialog::on_disableButton_clicked()
{
    int nCharityPercent = 0;
    CBitcoinAddress address = "";
    CBitcoinAddress changeAddress = "";
    qint64 nMinAmount = MIN_TX_FEE;
    qint64 nMaxAmount = MAX_MONEY;
    fGlobalStakeForCharity = false;

    model->setStakeForCharity(false, nCharityPercent, address, changeAddress, nMinAmount, nMaxAmount);
    ui->charityAddressEdit->clear();
    ui->charityChangeAddressEdit->clear();
    ui->charityMaxEdit->clear();
    ui->charityMinEdit->clear();
    ui->charityPercentEdit->clear();
    ui->comboBox->setCurrentIndex(0); // reset charity select combo
    ui->message->setStyleSheet("QLabel { color: #1ab06c; font-weight: 900;}");
    ui->message->setText(tr("SperoCoin Dynamic Stake for Charity is now off and saved off."));
    return;
}

void StakeForCharityDialog::on_comboBox_activated(int index)
{

}

void StakeForCharityDialog::on_comboBox_currentIndexChanged(int index)
{
    QPixmap IMGpixmap;
    QPixmap SPEROIMGpixmap;
    SPEROIMGpixmap.load(":/images/Wallet_Logo");
    if (index==0)
    {
        ui->charityAddressEdit->clear();
        ui->charityAddressEdit->setEnabled(true);
        ui->charityAddressEdit->setReadOnly(false);
        ui->addressBookButton->setDisabled(false);
        ui->charityAddressEdit->setStyleSheet("border-color: #000000; color: #000000;");
        ui->label_5->setStyleSheet("QLabel {color: #018457;}");
        ui->message->setText("Please enter the SperoCoin address <br />or select from the drop-down <br />and then click 'Enable'");
        ui->charityAddressEdit->setFocus();
        ui->label_IMG->clear();
        ui->label_HREF->clear();
    }
    else if (index==1)
    {
        ui->charityAddressEdit->clear();
        ui->charityAddressEdit->setDisabled(true);
        ui->charityAddressEdit->setStyleSheet("border-color: #35473c; color:35473c;");
        ui->label_5->setStyleSheet("QLabel {color: #018457;}");
        if (!fTestNet) ui->charityAddressEdit->setText(QString(FOUNDATION_POS));
        else  ui->charityAddressEdit->setText(QString(FOUNDATION_TEST_POS));
        ui->message->setText("Your donation will be used by the <a href='https://sperocoin.org/SPEROFoundation/' style='color: #1ab06c;'>SperoCoin Foundation, Inc.</a> <br />under the guidance of the board and community. <br />Click the 'Enable' button above to save <br />and start SperoCoin Dynamic Stake for Charity");
        ui->addressBookButton->setDisabled(true);
        ui->charityAddressEdit->setEnabled(false);
        ui->charityAddressEdit->setReadOnly(true);
        ui->label_IMG->setPixmap(SPEROIMGpixmap);
        ui->label_HREF->setText("<a href='https://sperocoin.org/SPEROFoundation/'><span style='text-decoration: underline; color:#1ab06c;'>Learn more</span></a>");
    }
    else if (index > 1)
    {
        ui->charityAddressEdit->clear();
        ui->charityAddressEdit->setText(charitiesAddress[index-1]);
        ui->charityAddressEdit->setEnabled(false);
        ui->charityAddressEdit->setReadOnly(true);
        ui->addressBookButton->setDisabled(true);
        ui->charityAddressEdit->setStyleSheet("border-color: #35473c; color:35473c;");
        ui->label_5->setStyleSheet("QLabel {color: #018457;}");
        ui->message->setText(charitiesAsk[index-1].replace("<a href","<a style='color: #1ab06c;' href") + "<br />Click the 'Enable' button above to save <br />and start SperoCoin Dynamic Stake for Charity");
        if (ui->btnRefreshCharities->isEnabled())  // ensure images do not load during refresh. Clearing the combo (part of refresh) changes the index, calling this function.
        {   // load the charity's image
            QSslConfiguration config = QSslConfiguration::defaultConfiguration();
            config.setProtocol(QSsl::TlsV1_2);
            QNetworkRequest imgRequest;
            imgRequest.setSslConfiguration(config);
            imgRequest.setUrl(QUrl(QString(charitiesImage[index-1])));
            imgRequest.setAttribute(
                        QNetworkRequest::CacheLoadControlAttribute,
                        QVariant(int(QNetworkRequest::AlwaysNetwork))
                        );
            QNetworkAccessManager nam2;
            QNetworkReply *reply = nam2.get(imgRequest);
            while(!reply->isFinished())
            {
                qApp->processEvents();
            }
            if (reply->error() == QNetworkReply::NoError)
            {
                QByteArray imgData =  reply->readAll();
                IMGpixmap.loadFromData(imgData);
                ui->label_IMG->setPixmap(IMGpixmap);
            }
            ui->label_HREF->setText("<a href='" + (QString)charitiesURL[index-1] +"'><span style='text-decoration: underline; color:#1ab06c;'>Learn more</span></a>");
        }
    }
}

void StakeForCharityDialog::on_btnRefreshCharities_clicked()
{
    ui->btnRefreshCharities->setText("Downloading...");
    ui->btnRefreshCharities->setDisabled(true);
    loadCharities();
    ui->charityAddressEdit->clear();
    ui->charityChangeAddressEdit->clear();
    ui->charityMaxEdit->clear();
    ui->charityMinEdit->clear();
    ui->charityPercentEdit->clear();
    ui->comboBox->setCurrentIndex(0); // reset charity select combo
    ui->message->setStyleSheet("QLabel { color: #1ab06c; font-weight: 900;}");
    ui->message->setText(tr("Data refreshed from <a href='https://sperocoin.org' style='color: #1ab06c;'>sperocoin.org</a><br />Select a cause you find charitable<br />and click the 'Enable' button to apply changes."));
    ui->btnRefreshCharities->setDisabled(false);
    ui->btnRefreshCharities->setText("Refresh Charities");
    ui->charityAddressEdit->setFocus();
}

void StakeForCharityDialog::updateMessageColor()
{
    WalletModel::EncryptionStatus status = model->getEncryptionStatus();
    if (status == WalletModel::Locked) ui->message->setStyleSheet("QLabel { color: red; font-weight: 900;}");
    else if (fWalletUnlockStakingOnly) ui->message->setStyleSheet("QLabel { color: red; font-weight: 900;}");
    else ui->message->setStyleSheet("QLabel { color: #1ab06c; font-weight: 900;}");
}

void StakeForCharityDialog::on_copyToClipboard_clicked()
{
    QApplication::clipboard()->setText(ui->charityAddressEdit->text());
}
