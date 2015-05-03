#include "XMLBuffer.h"
#include "XMLLexerTokens.h"
#include "macros.h"

wxStringSet_t XMLBuffer::m_emptyTags;

XMLBuffer::XMLBuffer(const wxString& buffer, bool htmlMode)
    : m_buffer(buffer)
    , m_state(kNormal)
    , m_htmlMode(htmlMode)
{
    m_scanner = ::xmlLexerNew(buffer);
}

XMLBuffer::~XMLBuffer()
{
    if(m_scanner) {
        ::xmlLexerDestroy(&m_scanner);
    }
}

void XMLBuffer::Parse()
{
    m_elements.clear();
    CHECK_PTR_RET(m_scanner);
    XMLLexerToken token;
    m_state = kNormal;
    while(::xmlLexerNext(m_scanner, token)) {
        switch(token.type) {
        case kXML_T_CDATA_START:
            m_state = kCdata;
            if(ConsumeUntil(kXML_T_CDATA_END)) {
                m_state = kNormal;
            }
            break;
        case kXML_T_COMMENT_START:
            m_state = kComment;
            if(ConsumeUntil(kXML_T_COMMENT_END)) {
                m_state = kNormal;
            }
            break;
        case kXML_T_OPEN_TAG:
            // "<" was found
            OnOpenTag();
            break;
        case kXML_T_CLOSE_TAG_PREFIX:
            // "</" was found
            OnTagClosePrefix();
            break;
        case kXML_T_CLOSE_TAG_SUFFIX:
            // "/>" closing tag was found
            if(!m_elements.empty()) m_elements.pop_back();
            break;
        default:
            break;
        }
    }
}

void XMLBuffer::OnOpenTag()
{
    XMLLexerToken token;
    if(!::xmlLexerNext(m_scanner, token)) return;
    if(token.type == kXML_T_IDENTIFIER) {
        // in html mode and found an empty tag?
        if(m_htmlMode && IsEmptyHtmlTag(token.text)) {
            return;
        }

        Scope scope;
        scope.line = token.lineNumber;
        scope.tag = token.text;
        m_elements.push_back(scope);
    }
}

void XMLBuffer::OnTagClosePrefix()
{
    XMLLexerToken token;
    if(!::xmlLexerNext(m_scanner, token)) return;
    if(token.type == kXML_T_IDENTIFIER) {
        if(!m_elements.empty()) {
            m_elements.pop_back();
        }
    }
}

XMLBuffer::Scope XMLBuffer::GetCurrentScope() const
{
    if(m_elements.empty()) return XMLBuffer::Scope();
    return m_elements.back();
}

bool XMLBuffer::ConsumeUntil(int until)
{
    XMLLexerToken token;
    while(::xmlLexerNext(m_scanner, token)) {
        if(token.type == until) return true;
    }
    return false;
}

bool XMLBuffer::IsEmptyHtmlTag(const wxString& tag)
{
    if(m_emptyTags.empty()) {
        m_emptyTags.insert("br");
        m_emptyTags.insert("hr");
        m_emptyTags.insert("meta");
        m_emptyTags.insert("link");
        m_emptyTags.insert("base");
        m_emptyTags.insert("img");
        m_emptyTags.insert("embed");
        m_emptyTags.insert("param");
        m_emptyTags.insert("area");
        m_emptyTags.insert("col");
        m_emptyTags.insert("input");
        m_emptyTags.insert("isindex");
        m_emptyTags.insert("basefont");
        m_emptyTags.insert("!doctype");
    }
    return m_emptyTags.count(tag.Lower());
}