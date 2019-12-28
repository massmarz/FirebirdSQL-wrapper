/* 
 * File:   Attachment.cpp
 * Created on 27 luglio 2018
 */

#include <utility> // std::move
#include "Attachment.h"

IMaster* master = fb_get_master_interface();

static IProvider* provider = master->getDispatcher();
static IUtil* util = master->getUtilInterface();

void formatExceptionMessage(const FbException& error, char *msg, unsigned int len) {
    util->formatStatus(msg, len, error.getStatus());
}

Attachment::Attachment() {
    status = new ThrowStatusWrapper(master->getStatus());
}

void Attachment::createDatabase(std::string server, std::string database, std::string username, std::string password, std::string charset) {
    if (att_)
        throw std::logic_error("Create database: disconnect before"); 

    setParameter(server, database, username, password, charset);
    
    att_ = provider->createDatabase(status, connectionString.c_str(),
    dpb->getBufferLength(status), dpb->getBuffer(status));
}

void Attachment::setParameter(std::string server, std::string database, std::string username, std::string password, std::string charset) {
    this->server = std::move(server);
    this->database = std::move(database);
    this->username = std::move(username);
    this->password = std::move(password);
    this->charset = std::move(charset);
    connectionString = this->server + ":" + this->database;

    if (dpb)
        dpb->dispose();

    dpb = util->getXpbBuilder(status, IXpbBuilder::DPB, NULL, 0);
    dpb->insertString(status, isc_dpb_user_name, this->username.c_str());
    dpb->insertString(status, isc_dpb_password, this->password.c_str());
    //dpb->insertString(status, isc_dpb_lc_ctype, this->charset.c_str());
    dpb->insertString(status, isc_dpb_set_db_charset, this->charset.c_str());
}

Attachment::~Attachment() {
    dispatcher.broadcast(DBStateEvents::ATTACHMENT_DELETE);

    if (att_) {
        att_->release();
    }

    if (provider)
        provider->release();

    if (status) {
        status->dispose();
        delete status;
    }

    if (dpb)
        dpb->dispose();
}

void Attachment::connect() {
    if (!att_) {
        att_ = provider->attachDatabase(status, connectionString.c_str(),
                dpb->getBufferLength(status), dpb->getBuffer(status));
    }
}

void Attachment::disconnect() {
    if (att_) {
        try {
            dispatcher.broadcast(DBStateEvents::ATTACHMENT_DISCONNECT);
            att_->detach(status);
        } catch (const FbException& e) {
            att_->release();
            
            char buf[256];
            formatExceptionMessage(e, buf, 256);
            fprintf(stderr, "%s\n", buf);
        }

        att_ = nullptr;
    }
}

IAttachment* Attachment::getHandle() {
    return att_;
}