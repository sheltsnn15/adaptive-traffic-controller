// Core/Src/tlv_parser.cpp
#include "tlv_parser.hpp"

TlvParser::TlvParser() : state_(ParseState::IDLE), pos_(0), expected_len_(0) {}

uint8_t TlvParser::parseByte(uint8_t byte) { return 0; }

bool TlvParser::validateFrame() const { return true; }

void TlvParser::dispatchMessage() {}

void TlvParser::reset() {
    state_ = ParseState::IDLE;
    pos_ = 0;
    expected_len_ = 0;
}
