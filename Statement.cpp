/* 
 * File:   Statement.cpp
 * Created on 27 luglio 2018
 */

#include <cstring> // memcpy, memset
#include <cmath> // floor
#include "Statement.h"

Statement::Statement() {
    status = new ThrowStatusWrapper(master->getStatus());
}

void Statement::setSql(std::string sql) {
    reset();

    this->sql = std::move(sql);
    initParametersByName();
    isPrepared = false;
}

void Statement::setTransaction(Transaction* transaction) {
    if (this->transaction) {
        transaction->dispatcher.removeCallBack(transactionCallbackID);
    }
    
    release();

    this->transaction = transaction;

    transactionCallbackID = transaction->dispatcher.addCallBack([this](DBStateEvents evt) {
        switch (evt) {
            case DBStateEvents::TRANSACTION_DISCONNECT:
                release();
                break;
            case DBStateEvents::TRANSACTION_DELETE:
                release();
                this->transaction = nullptr;
                break;
                
            default:
                break;
        }
    });

}

Statement::~Statement() {
    reset();

    if (stmt_) {
        stmt_->release();
    }

    if (status) {
        status->dispose();
        delete status;
    }
    if(transaction)
        transaction->dispatcher.removeCallBack(transactionCallbackID);
}

void Statement::release() {
    try {
        close();
    } catch (const FbException& e) {
        if (resSet_) {
            resSet_->release();
            resSet_ = nullptr;
        }

        char buf[256];
        formatExceptionMessage(e, buf, 256);
        fprintf(stderr, "%s\n", buf);
    }
    if (stmt_) {
        stmt_->release();
        stmt_ = nullptr;
    }
}

void Statement::reset() {
    if (stmt_) {
        stmt_->release();
        stmt_ = nullptr;
    }

    if (fields) {
        delete [] fields;
        fields = nullptr;
    }

    if (fieldsValueBuffer) {
        delete [] fieldsValueBuffer;
        fieldsValueBuffer = nullptr;
    }

    if (parametersValueBuffer) {
        delete [] parametersValueBuffer;
        parametersValueBuffer = nullptr;
    }

    if (resSet_) {
        resSet_->release();
        resSet_ = nullptr;
    }

    if (inMeta) {
        inMeta->release();
        inMeta = nullptr;
    }

    if (outMeta) {
        outMeta->release();
        outMeta = nullptr;
    }
}

void Statement::checkTransaction() {
    if (!transaction)
        throw std::logic_error("Statement: set transaction before !");

    transaction->connect();
}

void Statement::prepare() {
    //!! call checkTransaction before prepare()
    if (!isPrepared) {
        stmt_ = transaction->attachment->att_
                ->prepare(status,
                transaction->tra_, 0, sql.c_str(), SQL_DIALECT_V6,
                IStatement::PREPARE_PREFETCH_METADATA);

        outMeta = stmt_->getOutputMetadata(status);
        fieldsCount = outMeta->getCount(status);

        if (fieldsCount) {
            // allocate output buffer
            unsigned l = outMeta->getMessageLength(status);
            fieldsValueBuffer = new unsigned char[l];

            fields = new Field[fieldsCount];
            const char *fieldName;
            for (unsigned j = 0; j < fieldsCount; ++j) {
                fields[j].stmt = this;
                fields[j].type = outMeta->getType(status, j) & ~1;
                fields[j].length = outMeta->getLength(status, j);
                fields[j].offset = outMeta->getOffset(status, j);
                fields[j].nullOffset = outMeta->getNullOffset(status, j);

                fieldName = outMeta->getAlias(status, j);

                if (fieldName)
                    namedFields[fieldName] = j;
                else
                    namedFields[outMeta->getField(status, j)] = j;
            }
        }

        if (parametersCount) {
            inMeta = stmt_->getInputMetadata(status);
            inMeta->getCount(status);
            assert(parametersCount == inMeta->getCount(status));
            // allocate input buffer
            unsigned l = inMeta->getMessageLength(status);
            parametersValueBuffer = new unsigned char[l];
            parameters = new Parameter[parametersCount];
            for (unsigned j = 0; j < parametersCount; ++j) {
                parameters[j].stmt = this;
                parameters[j].type = inMeta->getType(status, j) & ~1;
                parameters[j].length = inMeta->getLength(status, j);
                parameters[j].offset = inMeta->getOffset(status, j);
                parameters[j].nullOffset = inMeta->getNullOffset(status, j);
            }
        }

        isPrepared = true;
    }
}

void Statement::open() {
    checkTransaction();

    if (!isPrepared)
        prepare();

    if (!stmt_)
        stmt_ = transaction->attachment->att_
            ->prepare(status,
            transaction->tra_, 0, sql.c_str(), SQL_DIALECT_V6, 0);

    resSet_ = stmt_->openCursor(status, transaction->tra_, inMeta, parametersValueBuffer, NULL, 0);
}

void Statement::execute() {
    checkTransaction();

    if (!isPrepared)
        prepare();

    if (!stmt_)
        stmt_ = transaction->attachment->att_
            ->prepare(status,
            transaction->tra_, 0, sql.c_str(), SQL_DIALECT_V6, 0);

    stmt_->execute(status, transaction->tra_, inMeta, parametersValueBuffer, NULL, NULL);

}

uint64_t Statement::getAffectedRecords() {
    if (stmt_)
        return stmt_->getAffectedRecords(status);
    else
        return 0;
}

void Statement::close() {
    if (resSet_) {
        resSet_->close(status);
        resSet_->release();
        resSet_ = nullptr;
    }
}

bool Statement::bof() {
    if(!resSet_)
        throw std::logic_error("Statement: call open before!");
        
    return resSet_->isBof(status) == FB_TRUE;
    
}

bool Statement::eof() {
    if(!resSet_)
        throw std::logic_error("Statement: call open before!");

    return resSet_->isEof(status) == FB_TRUE;
}

void Statement::next() {
    if(!resSet_)
        throw std::logic_error("Statement: call open before!");

    resSet_->fetchNext(status, fieldsValueBuffer);
}

bool Statement::fetch() {
    if(!resSet_)
        throw std::logic_error("Statement: call open before!");

    return resSet_->fetchNext(status, fieldsValueBuffer) == IStatus::RESULT_OK;
}

void Statement::initParametersByName() {
    // preprocess sql for named parameters
    int i = 0;
    int k = 0;
    parametersCount = 0;
    char paramName[32];
    while (sql[i] != '\0') {
        switch (sql[i]) {
            case '?':
                ++parametersCount;
                ++i;
                break;
            case '\'':
                ++i;
                while (sql[i] != '\0' && sql[i] != '\'')
                    ++i;
                if (sql[i] == '\0')
                    throw std::invalid_argument("invalid SQL statement !");
                else
                    ++i;
                break;
            case ':':
                k = 0;
                sql[i] = '?';
                ++i;
                while ((std::isalpha(sql[i]) || std::isdigit(sql[i]) || sql[i] == '_') && k < 32) {
                    paramName[k++] = sql[i];
                    sql[i] = ' ';
                    ++i;
                }
                if (k > 0 && k < 32) {
                    paramName[k] = '\0';
                    namedParameters[paramName] = parametersCount++;
                } else
                    throw std::invalid_argument("invalid SQL statement !");
                break;
            default:
                ++i;
                break;
        }
    }
}

/*********************************************************
 * Field 
 */
Statement::Field Statement::fieldByName(const char* name) {
    checkTransaction();

    if (!isPrepared)
        prepare();

    auto it = namedFields.find(name);

    if (it == namedFields.end())
        return {nullptr};

    return fields[it->second];

}

Statement::Field Statement::field(unsigned int idx) {
    checkTransaction();

    if (!isPrepared)
        prepare();

    if (idx >= fieldsCount)
        return {nullptr};
    return fields[idx];
}

Statement::Field::operator bool() const {
    return stmt != nullptr;
}

int64_t Statement::Field::asInteger() const {
    assert(stmt);

    if (*((short*) (stmt->fieldsValueBuffer + nullOffset))) {
        return 0;
    }

    int64_t ret;
    std::string tmp;

    switch (type) {
        case SQL_TEXT:
            tmp.assign((char*) (stmt->fieldsValueBuffer + offset), length);
            ret = strtoll(tmp.c_str(), nullptr, 0);
            break;
        case SQL_VARYING:
            tmp.assign((char*) (stmt->fieldsValueBuffer + offset + sizeof (short)), length);
            ret = strtoll(tmp.c_str(), nullptr, 0);
            break;
        case SQL_SHORT:
            ret = *((const ISC_SHORT*) (stmt->fieldsValueBuffer + offset));
            break;
        case SQL_LONG:
            ret = *((const ISC_LONG*) (stmt->fieldsValueBuffer + offset));
            break;
        case SQL_INT64:
            ret = *((const ISC_INT64*) (stmt->fieldsValueBuffer + offset));
            break;
        case SQL_FLOAT:
            ret = *((const float*) (stmt->fieldsValueBuffer + offset));
            break;
        case SQL_DOUBLE:
            ret = *((const double*) (stmt->fieldsValueBuffer + offset));
            break;
        case SQL_NULL:
        default:
            ret = 0;
            break;
    }
    return ret;
}

std::string Statement::Field::asString() const {
    assert(stmt);

    if (*((short*) (stmt->fieldsValueBuffer + nullOffset))) {
        return std::string();
    }

    std::string ret;

    switch (type) {
        case SQL_TEXT:
            ret.assign((char*) (stmt->fieldsValueBuffer + offset), length);
            break;
        case SQL_VARYING:
            ret.assign((char*) (stmt->fieldsValueBuffer + offset + sizeof (short)), length);
            break;
        case SQL_SHORT:
            ret = std::to_string(*((const ISC_SHORT*) (stmt->fieldsValueBuffer + offset)));
            break;
        case SQL_LONG:
            ret = std::to_string(*((const ISC_LONG*) (stmt->fieldsValueBuffer + offset)));
            break;
        case SQL_INT64:
            ret = std::to_string(*((const ISC_INT64*) (stmt->fieldsValueBuffer + offset)));
            break;
        case SQL_FLOAT:
            ret = std::to_string(*((const float*) (stmt->fieldsValueBuffer + offset)));
            break;
        case SQL_DOUBLE:
            ret = std::to_string(*((const float*) (stmt->fieldsValueBuffer + offset)));
            break;
        case SQL_NULL:
        default:
            break;
    }
    return ret;
}

double Statement::Field::asDouble() const {
    assert(stmt);

    if (*((short*) (stmt->fieldsValueBuffer + nullOffset))) {
        return 0;
    }

    double ret;
    std::string tmp;

    switch (type) {
        case SQL_SHORT:
            ret = *((const ISC_SHORT*) (stmt->fieldsValueBuffer + offset));
            break;
        case SQL_LONG:
            ret = *((const ISC_LONG*) (stmt->fieldsValueBuffer + offset));
            break;
        case SQL_INT64:
            ret = *((const ISC_INT64*) (stmt->fieldsValueBuffer + offset));
            break;
        case SQL_FLOAT:
            ret = *((const float*) (stmt->fieldsValueBuffer + offset));
            break;
        case SQL_DOUBLE:
            ret = *((const double*) (stmt->fieldsValueBuffer + offset));
            break;
        case SQL_TEXT:
            tmp.assign((char*) (stmt->fieldsValueBuffer + offset), length);
            ret = strtod(tmp.c_str(), nullptr);
            break;
        case SQL_VARYING:
            tmp.assign((char*) (stmt->fieldsValueBuffer + offset + sizeof (short)), length);
            ret = strtod(tmp.c_str(), nullptr);
            break;
        default:
            ret = 0;
            break;
    }
    return ret;
}

std::string Statement::Field::formatDate(const std::string &format) const {
    struct tm times;
    char buff[30];

    assert(stmt);

    if (*((short*) (stmt->fieldsValueBuffer + nullOffset))) {
        return 0;
    }

    switch (type) {
        case SQL_TYPE_DATE:
            isc_decode_sql_date((const ISC_DATE*) (stmt->fieldsValueBuffer + offset), &times);
            strftime(buff, 30, format.c_str(), &times);
            break;
        case SQL_TIMESTAMP:
            isc_decode_timestamp((const ISC_TIMESTAMP*) (stmt->fieldsValueBuffer + offset), &times);
            strftime(buff, 30, format.c_str(), &times);
            break;
        default:
            return std::string();
            break;
    }

    return buff;
}

/*********************************************************
 * Parameter
 */
Statement::Parameter Statement::paramByName(const char* name) {
    checkTransaction();

    if (!isPrepared)
        prepare();

    auto it = namedParameters.find(name);

    if (it == namedParameters.end())
        return {nullptr};

    return parameters[it->second];
}

Statement::Parameter Statement::parameter(unsigned int idx) {
    checkTransaction();

    if (!isPrepared)
        prepare();

    if (idx >= parametersCount)
        return {nullptr};
    return parameters[idx];
}

Statement::Parameter::operator bool() const {
    return stmt != nullptr;
}

void Statement::Parameter::setInt(uint64_t v) {
    assert(stmt);

    *((short*) (stmt->parametersValueBuffer + nullOffset)) = 0;

    switch (type) {
        case SQL_SHORT:
            *((ISC_SHORT*) (stmt->parametersValueBuffer + offset)) = v;
            break;
        case SQL_LONG:
            *((ISC_LONG*) (stmt->parametersValueBuffer + offset)) = v;
            break;
        case SQL_INT64:
            *((ISC_INT64*) (stmt->parametersValueBuffer + offset)) = v;
            break;
        default:
            throw std::invalid_argument("Binding parameter: invalid data type!");
            break;
    }
}

void Statement::Parameter::setDouble(double v) {
    assert(stmt);

    *((short*) (stmt->parametersValueBuffer + nullOffset)) = 0;

    switch (type) {
        case SQL_FLOAT:
            *((float*) (stmt->parametersValueBuffer + offset)) = v;
            break;
        case SQL_DOUBLE:
            *((double*) (stmt->parametersValueBuffer + offset)) = v;
            break;
        case SQL_SHORT:
            *((ISC_SHORT*) (stmt->parametersValueBuffer + offset)) = (ISC_SHORT) std::floor(v + 0.5);
            break;
        case SQL_LONG:
            *((ISC_LONG*) (stmt->parametersValueBuffer + offset)) = (ISC_LONG) std::floor(v + 0.5);
            break;
        case SQL_INT64:
            *((ISC_INT64*) (stmt->parametersValueBuffer + offset)) = (ISC_INT64) std::floor(v + 0.5);
            break;
        default:
            throw std::invalid_argument("Binding parameter: invalid data type!");
            break;
    }
}

void Statement::Parameter::setText(const char* v) {
    assert(stmt);

    unsigned short len = std::strlen(v);

    assert(len <= length);

    *((short*) (stmt->parametersValueBuffer + nullOffset)) = 0;

    char *p;

    switch (type) {
        case SQL_TEXT:
            p = (char*) (stmt->parametersValueBuffer + offset);
            std::memcpy(p, v, len);
            if (len < length) {
                p += len;
                std::memset(p, ' ', length - len);
            }
            break;
        case SQL_VARYING:
            *(short*) (stmt->parametersValueBuffer + offset) = len;
            p = (char*) (stmt->parametersValueBuffer + offset + sizeof (short));
            std::memcpy(p, v, len);
            break;

        default:
            throw std::invalid_argument("Binding parameter: invalid data type!");
            break;
    }
}

void Statement::Parameter::setNull() {
    *((short*) (stmt->parametersValueBuffer + nullOffset)) = 1;
}