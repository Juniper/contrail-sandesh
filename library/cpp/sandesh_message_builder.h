/*
 * Copyright (c) 2014 Juniper Networks, Inc. All rights reserved.
 */

#ifndef __SANDESH_MESSAGE_BUILDER_H__
#define __SANDESH_MESSAGE_BUILDER_H__

#include <pugixml/pugixml.hpp>

#include <sandesh/sandesh_types.h>
#include <sandesh/sandesh.h>

class SandeshMessage {
public:
    SandeshMessage() :
        size_(0) {
    }
    virtual ~SandeshMessage();
    virtual bool Parse(const uint8_t *data, size_t size) = 0;
    virtual const std::string ExtractMessage() const = 0;
    const SandeshHeader& GetHeader() const { return header_; }
    const std::string& GetMessageType() const { return message_type_; }
    const size_t GetSize() const { return size_; }
 
protected:
    SandeshHeader header_;
    std::string message_type_;
    size_t size_;
};

class SandeshXMLMessage : public SandeshMessage {
public:
    SandeshXMLMessage() {}
    virtual ~SandeshXMLMessage();
    virtual bool Parse(const uint8_t *data, size_t size);
    virtual const std::string ExtractMessage() const;
    const pugi::xml_node& GetMessageNode() const { return message_node_; }

protected:
    bool ParseHeader(const pugi::xml_node& root,
        SandeshHeader& header);

    pugi::xml_document xdoc_;
    pugi::xml_node message_node_;

private:
    DISALLOW_COPY_AND_ASSIGN(SandeshXMLMessage);
};

class SandeshSyslogMessage : public SandeshXMLMessage {
public:
    SandeshSyslogMessage() {}
    virtual ~SandeshSyslogMessage();
    virtual bool Parse(const uint8_t *data, size_t size);
    void SetHeader(const SandeshHeader &header) { header_ = header; }

private:
    DISALLOW_COPY_AND_ASSIGN(SandeshSyslogMessage);
};
    
class SandeshMessageBuilder {
public:
    enum Type {
        XML,
        SYSLOG,
    };
    virtual SandeshMessage *Create(const uint8_t *data, size_t size) const = 0;
    static SandeshMessageBuilder *GetInstance(Type type);
};

class SandeshXMLMessageBuilder : public SandeshMessageBuilder {
public:
    SandeshXMLMessageBuilder();
    virtual SandeshMessage *Create(const uint8_t *data, size_t size) const;
    static SandeshXMLMessageBuilder *GetInstance();

private:
    static SandeshXMLMessageBuilder instance_;
    DISALLOW_COPY_AND_ASSIGN(SandeshXMLMessageBuilder);
};

class SandeshSyslogMessageBuilder : public SandeshMessageBuilder {
public:
    SandeshSyslogMessageBuilder();
    virtual SandeshMessage *Create(const uint8_t *data, size_t size) const;
    static SandeshSyslogMessageBuilder *GetInstance();

private:
    static SandeshSyslogMessageBuilder instance_;
    DISALLOW_COPY_AND_ASSIGN(SandeshSyslogMessageBuilder);
};

#endif // __SANDESH_MESSAGE_BUILDER_H__
