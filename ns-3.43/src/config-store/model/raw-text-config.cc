/*
 * Copyright (c) 2009 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@cutebugs.net>
 */

#include "raw-text-config.h"

#include "attribute-default-iterator.h"
#include "attribute-iterator.h"

#include "ns3/config.h"
#include "ns3/global-value.h"
#include "ns3/log.h"
#include "ns3/string.h"

#include <algorithm>
#include <functional>
#include <istream>
#include <sstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RawTextConfig");

RawTextConfigSave::RawTextConfigSave()
    : m_os(nullptr)
{
    NS_LOG_FUNCTION(this);
}

RawTextConfigSave::~RawTextConfigSave()
{
    NS_LOG_FUNCTION(this);
    if (m_os != nullptr)
    {
        m_os->close();
    }
    delete m_os;
    m_os = nullptr;
}

void
RawTextConfigSave::SetFilename(std::string filename)
{
    NS_LOG_FUNCTION(this << filename);
    m_os = new std::ofstream();
    m_os->open(filename, std::ios::out);
}

void
RawTextConfigSave::Default()
{
    NS_LOG_FUNCTION(this);

    class RawTextDefaultIterator : public AttributeDefaultIterator
    {
      public:
        RawTextDefaultIterator(std::ostream* os)
        {
            m_os = os;
        }

      private:
        void StartVisitTypeId(std::string name) override
        {
            m_typeId = name;
        }

        void DoVisitAttribute(std::string name, std::string defaultValue) override
        {
            NS_LOG_DEBUG("Saving " << m_typeId << "::" << name);
            TypeId tid = TypeId::LookupByName(m_typeId);
            ns3::TypeId::SupportLevel supportLevel = TypeId::SupportLevel::SUPPORTED;
            std::string originalInitialValue;
            std::string valueTypeName;
            for (std::size_t i = 0; i < tid.GetAttributeN(); i++)
            {
                TypeId::AttributeInformation tmp = tid.GetAttribute(i);
                if (tmp.name == name)
                {
                    supportLevel = tmp.supportLevel;
                    originalInitialValue = tmp.originalInitialValue->SerializeToString(tmp.checker);
                    valueTypeName = tmp.checker->GetValueTypeName();
                    break;
                }
            }
            if (valueTypeName == "ns3::CallbackValue")
            {
                NS_LOG_WARN("Global attribute " << m_typeId << "::" << name
                                                << " was not saved because it is a CallbackValue");
                return;
            }
            if (supportLevel == TypeId::SupportLevel::OBSOLETE)
            {
                NS_LOG_WARN("Global attribute " << m_typeId << "::" << name
                                                << " was not saved because it is OBSOLETE");
                return;
            }
            if (supportLevel == TypeId::SupportLevel::DEPRECATED &&
                defaultValue == originalInitialValue)
            {
                NS_LOG_WARN("Global attribute "
                            << m_typeId << "::" << name
                            << " was not saved because it is DEPRECATED and its value has not "
                               "changed from the original initial value");
                return;
            }
            *m_os << "default " << m_typeId << "::" << name << " \"" << defaultValue << "\""
                  << std::endl;
        }

        std::string m_typeId;
        std::ostream* m_os;
    };

    RawTextDefaultIterator iterator = RawTextDefaultIterator(m_os);
    iterator.Iterate();
}

void
RawTextConfigSave::Global()
{
    NS_LOG_FUNCTION(this);
    for (auto i = GlobalValue::Begin(); i != GlobalValue::End(); ++i)
    {
        StringValue value;
        (*i)->GetValue(value);
        NS_LOG_LOGIC("Saving " << (*i)->GetName());
        *m_os << "global " << (*i)->GetName() << " \"" << value.Get() << "\"" << std::endl;
    }
}

void
RawTextConfigSave::Attributes()
{
    NS_LOG_FUNCTION(this);

    class RawTextAttributeIterator : public AttributeIterator
    {
      public:
        RawTextAttributeIterator(std::ostream* os)
            : m_os(os)
        {
        }

      private:
        void DoVisitAttribute(Ptr<Object> object, std::string name) override
        {
            StringValue str;
            TypeId tid = object->GetInstanceTypeId();

            auto [found, inTid, attr] = TypeId::FindAttribute(tid, name);

            if (found)
            {
                if (attr.checker && attr.checker->GetValueTypeName() == "ns3::CallbackValue")
                {
                    NS_LOG_WARN("Attribute " << GetCurrentPath()
                                             << " was not saved because it is a CallbackValue");
                    return;
                }
                auto supportLevel = attr.supportLevel;
                if (supportLevel == TypeId::SupportLevel::OBSOLETE)
                {
                    NS_LOG_WARN("Attribute " << GetCurrentPath()
                                             << " was not saved because it is OBSOLETE");
                    return;
                }

                std::string originalInitialValue =
                    attr.originalInitialValue->SerializeToString(attr.checker);
                object->GetAttribute(name, str, true);

                if (supportLevel == TypeId::SupportLevel::DEPRECATED &&
                    str.Get() == originalInitialValue)
                {
                    NS_LOG_WARN("Attribute "
                                << GetCurrentPath()
                                << " was not saved because it is DEPRECATED and its value has not "
                                   "changed from the original initial value");
                    return;
                }
                NS_LOG_DEBUG("Saving " << GetCurrentPath());
                *m_os << "value " << GetCurrentPath() << " \"" << str.Get() << "\"" << std::endl;
            }
        }

        std::ostream* m_os;
    };

    RawTextAttributeIterator iter = RawTextAttributeIterator(m_os);
    iter.Iterate();
}

RawTextConfigLoad::RawTextConfigLoad()
    : m_is(nullptr)
{
    NS_LOG_FUNCTION(this);
}

RawTextConfigLoad::~RawTextConfigLoad()
{
    NS_LOG_FUNCTION(this);
    if (m_is != nullptr)
    {
        m_is->close();
        delete m_is;
        m_is = nullptr;
    }
}

void
RawTextConfigLoad::SetFilename(std::string filename)
{
    NS_LOG_FUNCTION(this << filename);
    m_is = new std::ifstream();
    m_is->open(filename, std::ios::in);
}

std::string
RawTextConfigLoad::Strip(std::string value)
{
    NS_LOG_FUNCTION(this << value);
    std::string::size_type start = value.find('\"');
    std::string::size_type end = value.find('\"', 1);
    NS_ABORT_MSG_IF(start != 0, "Ill-formed attribute value: " << value);
    NS_ABORT_MSG_IF(end != value.size() - 1, "Ill-formed attribute value: " << value);
    return value.substr(start + 1, end - start - 1);
}

void
RawTextConfigLoad::Default()
{
    NS_LOG_FUNCTION(this);
    m_is->clear();
    m_is->seekg(0);
    std::string type;
    std::string name;
    std::string value;
    for (std::string line; std::getline(*m_is, line);)
    {
        if (!ParseLine(line, type, name, value))
        {
            continue;
        }

        NS_LOG_DEBUG("type=" << type << ", name=" << name << ", value=" << value);
        value = Strip(value);
        if (type == "default")
        {
            Config::SetDefault(name, StringValue(value));
        }
        name.clear();
        type.clear();
        value.clear();
    }
}

void
RawTextConfigLoad::Global()
{
    NS_LOG_FUNCTION(this);
    m_is->clear();
    m_is->seekg(0);
    std::string type;
    std::string name;
    std::string value;
    for (std::string line; std::getline(*m_is, line);)
    {
        if (!ParseLine(line, type, name, value))
        {
            continue;
        }

        NS_LOG_DEBUG("type=" << type << ", name=" << name << ", value=" << value);
        value = Strip(value);
        if (type == "global")
        {
            Config::SetGlobal(name, StringValue(value));
        }
        name.clear();
        type.clear();
        value.clear();
    }
}

void
RawTextConfigLoad::Attributes()
{
    NS_LOG_FUNCTION(this);
    m_is->clear();
    m_is->seekg(0);
    std::string type;
    std::string name;
    std::string value;
    for (std::string line; std::getline(*m_is, line);)
    {
        if (!ParseLine(line, type, name, value))
        {
            continue;
        }

        NS_LOG_DEBUG("type=" << type << ", name=" << name << ", value=" << value);
        value = Strip(value);
        if (type == "value")
        {
            Config::Set(name, StringValue(value));
        }
        name.clear();
        type.clear();
        value.clear();
    }
}

bool
RawTextConfigLoad::ParseLine(const std::string& line,
                             std::string& type,
                             std::string& name,
                             std::string& value)
{
    NS_LOG_FUNCTION(this << line << type << name << value);

    // check for blank line
    {
        std::istringstream iss(line);
        iss >> std::ws;  // remove all blanks line
        if (!iss.good()) // eofbit set if no non-blanks
        {
            return false;
        }
    }

    if (line.front() == '#')
    {
        return false; // comment line
    }

    // for multiline values, append line to value if type and name not empty
    if (type.empty() && name.empty())
    {
        std::istringstream iss(line);
        iss >> type >> name >> std::ws;
        std::getline(iss, value); // remaining line, includes embedded spaces
    }
    else
    {
        value.append(line);
    }

    // two quotes in value signifies a completed (possibly multi-line)
    // config-store entry, return True to signal load function to
    // validate value (see Strip method) and set attribute
    return std::count(value.begin(), value.end(), '"') == 2;
}

} // namespace ns3
