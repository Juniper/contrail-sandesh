/*
 * Copyright (c) 2014 Juniper Networks, Inc. All rights reserved.
 */

#include <sandesh/sandesh_message_builder.h>

using namespace pugi;
using namespace std;

// SandeshMessage
SandeshMessage::~SandeshMessage() {
}

static bool ParseInteger(const pugi::xml_node &node, int *valuep) {
    char *endp;
    *valuep = strtoul(node.child_value(), &endp, 10);
    return endp[0] == '\0';
}

static bool ParseUnsignedLong(const pugi::xml_node &node, uint64_t *valuep) {
    char *endp;
    *valuep = strtoull(node.child_value(), &endp, 10);
    return endp[0] == '\0';
}

// SandeshXMLMessage
SandeshXMLMessage::~SandeshXMLMessage() {
}

bool SandeshXMLMessage::ParseHeader(const xml_node& root,
    SandeshHeader& header) {
    for (xml_node node = root.first_child(); node;
         node = node.next_sibling()) {
        const char *name(node.name());
        assert(strcmp(node.last_attribute().name(), "identifier") == 0);
        int identifier(node.last_attribute().as_int());
        switch (identifier) {
        case 1:
            header.set_Namespace(node.child_value());
            break;
        case 2:
            uint64_t Timestamp;
            if (!ParseUnsignedLong(node, &Timestamp)) return false;
            header.set_Timestamp(Timestamp);
            break;
        case 3:
            header.set_Module(node.child_value());
            break;
        case 4:
            header.set_Source(node.child_value());
            break;
        case 5:
            header.set_Context(node.child_value());
            break;
        case 6:
            int SequenceNum;
            if (!ParseInteger(node, &SequenceNum)) return false;
            header.set_SequenceNum(SequenceNum);
            break;
        case 7:
            int VersionSig;
            if (!ParseInteger(node, &VersionSig)) return false;
            header.set_VersionSig(VersionSig);
            break;
        case 8:
            int Type;
            if (!ParseInteger(node, &Type)) return false;
            header.set_Type(static_cast<SandeshType::type>(Type));
            break;
        case 9:
            int Hints;
            if (!ParseInteger(node, &Hints)) return false;
            header.set_Hints(Hints);
            break;
        case 10:
            int Level;
            if (!ParseInteger(node, &Level)) return false;
            header.set_Level(static_cast<SandeshLevel::type>(Level));
            break;
        case 11:
            header.set_Category(node.child_value());
            break;
        case 12:
            header.set_NodeType(node.child_value());
            break;
        case 13:
            header.set_InstanceId(node.child_value());
            break;
        case 14:
            header.set_IPAddress(node.child_value());
            break;
        case 15:
            int Pid;
            if (!ParseInteger(node, &Pid)) return false;
            header.set_Pid(Pid);
            break;
        default:
            SANDESH_LOG(ERROR, __func__ << ": Unknown identifier: " << identifier);
            break;
        }
    }
    return true;
}

bool SandeshXMLMessage::Parse(const uint8_t *xml_msg, size_t size) {
    xml_parse_result result = xdoc_.load_buffer(xml_msg, size,
        parse_default & ~parse_escapes);
    if (!result) {
        SANDESH_LOG(ERROR, __func__ << ": Unable to load Sandesh XML. (status=" <<
            result.status << ", offset=" << result.offset << "): " << 
            xml_msg);
        return false;
    }
    xml_node header_node = xdoc_.first_child();
    if (!ParseHeader(header_node, header_)) {
        SANDESH_LOG(ERROR, __func__ << ": Sandesh header parse FAILED: " <<
            xml_msg);
        return false;
    }
    message_node_ = header_node.next_sibling();
    message_type_ = message_node_.name();
    if (message_type_.empty()) {
        SANDESH_LOG(ERROR, __func__ << ": Message type NOT PRESENT: " <<
            xml_msg);
        return false;
    }
    size_ = size;
    return true;
}

const std::string SandeshXMLMessage::ExtractMessage() const {
    ostringstream sstream;
    message_node_.print(sstream, "", format_raw | format_no_declaration | 
        format_no_escapes);
    return sstream.str();
}

// SandeshSyslogMessage
SandeshSyslogMessage::~SandeshSyslogMessage() {
}

bool SandeshSyslogMessage::Parse(const uint8_t *xml_msg, size_t size) {
    xml_parse_result result = xdoc_.load_buffer(xml_msg, size,
        parse_default & ~parse_escapes);
    if (!result) {
        SANDESH_LOG(ERROR, __func__ << ": Unable to load Sandesh Syslog. (status=" <<
            result.status << ", offset=" << result.offset << "): " <<
            xml_msg);
        return false;
    }
    message_node_ = xdoc_.first_child();
    message_type_ = message_node_.name();
    size_ = size;
}

// SandeshMessageBuilder
SandeshMessageBuilder *SandeshMessageBuilder::GetInstance(
    SandeshMessageBuilder::Type type) {
    if (type == SandeshMessageBuilder::XML) {
        return SandeshXMLMessageBuilder::GetInstance();
    } else if (type == SandeshMessageBuilder::SYSLOG) {
        return SandeshSyslogMessageBuilder::GetInstance();
    }
    return NULL;
}

// SandeshXMLMessageBuilder
SandeshMessage *SandeshXMLMessageBuilder::Create(
    const uint8_t *xml_msg, size_t size) const {
    SandeshXMLMessage *msg = new SandeshXMLMessage;
    if (!msg->Parse(xml_msg, size)) {
        delete msg;
        return NULL;
    }
    return msg;
}

SandeshXMLMessageBuilder SandeshXMLMessageBuilder::instance_;

SandeshXMLMessageBuilder::SandeshXMLMessageBuilder() {
}

SandeshXMLMessageBuilder *SandeshXMLMessageBuilder::GetInstance() {
    return &instance_;
}

// SandeshSyslogMessageBuilder
SandeshMessage *SandeshSyslogMessageBuilder::Create(
    const uint8_t *xml_msg, size_t size) const {
    SandeshSyslogMessage *msg = new SandeshSyslogMessage;
    msg->Parse(xml_msg, size);
    return msg;
}

SandeshSyslogMessageBuilder SandeshSyslogMessageBuilder::instance_;

SandeshSyslogMessageBuilder::SandeshSyslogMessageBuilder() {
}

SandeshSyslogMessageBuilder *SandeshSyslogMessageBuilder::GetInstance() {
    return &instance_;
}
