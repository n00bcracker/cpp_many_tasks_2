#include "resp_writer.h"
#include <string>
#include <string_view>
#include "cactus/io/view.h"
#include "../resp_types.h"

namespace redis {

void RespWriter::FinishMessageField() {
    std::string end_string("\r\n");
    writer_->Write(cactus::View(end_string));
}

void RespWriter::WriteSimpleString(std::string_view s) {
    writer_->Write(cactus::View(RespTypeToChar(ERespType::SimpleString)));
    writer_->Write(cactus::View(s));
    FinishMessageField();
}

void RespWriter::WriteError(std::string_view s) {
    writer_->Write(cactus::View(RespTypeToChar(ERespType::Error)));
    writer_->Write(cactus::View(s));
    FinishMessageField();
}

void RespWriter::WriteInt(int64_t n) {
    writer_->Write(cactus::View(RespTypeToChar(ERespType::Int)));
    writer_->Write(cactus::View(std::to_string(n)));
    FinishMessageField();
}

void RespWriter::WriteBulkString(std::string_view s) {
    writer_->Write(cactus::View(RespTypeToChar(ERespType::BulkString)));
    writer_->Write(cactus::View(std::to_string(s.size())));
    FinishMessageField();
    writer_->Write(cactus::View(s));
    FinishMessageField();
}

void RespWriter::WriteNullBulkString() {
    writer_->Write(cactus::View(RespTypeToChar(ERespType::BulkString)));
    writer_->Write(cactus::View(std::string("-1")));
    FinishMessageField();
}

void RespWriter::StartArray(size_t size) {
    writer_->Write(cactus::View(RespTypeToChar(ERespType::Array)));
    writer_->Write(cactus::View(std::to_string(size)));
    FinishMessageField();
}

void RespWriter::WriteNullArray() {
    writer_->Write(cactus::View(RespTypeToChar(ERespType::Array)));
    writer_->Write(cactus::View(std::string("-1")));
    FinishMessageField();
}

}  // namespace redis
