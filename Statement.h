/* 
 * File:   Statement.h
 * Created on 27 luglio 2018
 */

#ifndef STATEMENT_H
#define STATEMENT_H

#include <unordered_map>
#include "Transaction.h"

class Statement {
public:
    // class to cache received metadata
    class Field {
    public:
        Statement* stmt = nullptr;
        unsigned type = 0;
        unsigned length = 0;
        unsigned offset = 0;
        unsigned nullOffset= 0;
        explicit operator bool() const;
        int64_t asInteger() const;
        double asDouble() const;
        std::string asString() const;
        std::string formatDate(const std::string &format = "%Y-%m-%d") const;
    };

    class Parameter {
    public:
        Statement* stmt;
        unsigned type = 0;
        unsigned length = 0;
        unsigned offset = 0;
        unsigned nullOffset= 0;

        explicit operator bool() const;

        void setInt(uint64_t v);
        void setDouble(double v);
        void setText(const char* v);
        void setNull();
    };

    Statement();
    void setSql(std::string sql);
    void setTransaction(Transaction* transaction);

    Field fieldByName(const char* name);
    Field field(unsigned int idx);
            
    Parameter paramByName(const char* name);
    Parameter parameter(unsigned int idx);

    virtual ~Statement();
    void open();
    void execute();
    void close();
    void reset();
    bool fetch();
    bool bof();
    bool eof();
    void next();
    uint64_t getAffectedRecords();
private:
    void checkTransaction();
    void prepare();
    void initParametersByName();
    void release();

    Parameter* parameters = nullptr;
    std::unordered_map<std::string, unsigned int> namedParameters;
    unsigned int parametersCount = 0;
    unsigned char* parametersValueBuffer = nullptr;

    Field* fields = nullptr;
    std::unordered_map<std::string, unsigned int> namedFields;
    unsigned int fieldsCount = 0;
    unsigned char* fieldsValueBuffer = nullptr;
    
    std::string sql;
    Transaction* transaction;
    IStatement* stmt_ = nullptr;
    ThrowStatusWrapper* status = nullptr;
    IResultSet* resSet_ = nullptr;
    IMessageMetadata* inMeta = nullptr;
    IMessageMetadata* outMeta = nullptr;

    bool isPrepared = false;
    
    EventDispatcher<DBStateEvents>::CBID transactionCallbackID;
};

#endif /* STATEMENT_H */

