#include "../runtime/gw_portal_xml.h"
#include <windows.h>
#include <shlwapi.h>
#include <xmllite.h>
#include <wrl/client.h>
#include <map>
#include <string>
#include <string_view>
#include <stdexcept>

namespace fw::gw::portal {
std::map<std::string, std::string> request_fields(std::string_view body) {
    if (body.empty() || body.size() > 4096) throw std::invalid_argument("XML body size");
    using Microsoft::WRL::ComPtr;
    ComPtr<IStream> stream;
    stream.Attach(SHCreateMemStream(reinterpret_cast<const BYTE*>(body.data()), static_cast<UINT>(body.size())));
    ComPtr<IXmlReader> reader;
    auto check = [](HRESULT value) { if (FAILED(value)) throw std::invalid_argument("Invalid portal XML"); };
    check(CreateXmlReader(__uuidof(IXmlReader), reinterpret_cast<void**>(reader.GetAddressOf()), nullptr));
    check(reader->SetProperty(XmlReaderProperty_DtdProcessing, DtdProcessing_Prohibit));
    check(reader->SetProperty(XmlReaderProperty_MaxElementDepth, 3));
    check(reader->SetInput(stream.Get()));
    auto utf8 = [](const wchar_t* text, UINT length) {
        if (!length) return std::string{};
        auto count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text, int(length), nullptr, 0, nullptr, nullptr);
        if (!count) throw std::invalid_argument("XML encoding");
        std::string value(count, '\0');
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text, int(length), value.data(), count, nullptr, nullptr);
        return value;
    };
    std::map<std::string, std::string> fields;
    std::string current;
    bool root = false, closed = false;
    XmlNodeType type;
    HRESULT result;
    while ((result = reader->Read(&type)) == S_OK) {
        UINT depth = 0; check(reader->GetDepth(&depth));
        if (type == XmlNodeType_Element) {
            const wchar_t* name; UINT length; check(reader->GetQualifiedName(&name, &length));
            auto tag = utf8(name, length);
            UINT attributes = 0; check(reader->GetAttributeCount(&attributes));
            if (attributes || closed) throw std::invalid_argument("Unexpected XML attributes or root");
            if (depth == 0) {
                if (root || tag != "Request") throw std::invalid_argument("Request root required");
                root = true; closed = reader->IsEmptyElement();
            } else if (depth == 1) {
                if (!root || !fields.emplace(tag, "").second) throw std::invalid_argument("Duplicate XML field");
                current = reader->IsEmptyElement() ? "" : tag;
            } else if (depth == 2 && current == "CreationFlags" && reader->IsEmptyElement() &&
                       (tag == "NoPortalAccount" || tag == "NoGameAccount")) {
                // Client hints are accepted, but this listener never auto-registers.
            } else throw std::invalid_argument("Nested XML field");
        } else if (type == XmlNodeType_EndElement) {
            const wchar_t* name; UINT length; check(reader->GetQualifiedName(&name, &length));
            auto tag = utf8(name, length);
            if (tag == "Request") closed = true;
            else if (tag == current) current.clear();
        } else if (type == XmlNodeType_Text || type == XmlNodeType_CDATA || type == XmlNodeType_Whitespace) {
            const wchar_t* value; UINT length; check(reader->GetValue(&value, &length));
            if (depth == 2 && !current.empty() && current != "CreationFlags") fields.at(current) += utf8(value, length);
            else if (type != XmlNodeType_Whitespace)
                throw std::invalid_argument("Unexpected XML text");
        } else if (type != XmlNodeType_XmlDeclaration) throw std::invalid_argument("Unexpected XML node");
    }
    check(result);
    if (!root || !closed) throw std::invalid_argument("Incomplete XML request");
    return fields;
}
}



