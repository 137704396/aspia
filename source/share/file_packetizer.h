//
// Aspia Project
// Copyright (C) 2018 Dmitry Chapyshev <dmitry@aspia.ru>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//

#ifndef ASPIA_SHARE__FILE_PACKETIZER_H_
#define ASPIA_SHARE__FILE_PACKETIZER_H_

#include <filesystem>
#include <fstream>
#include <memory>

#include "base/macros_magic.h"
#include "protocol/file_transfer_session.pb.h"

namespace aspia {

class FilePacketizer
{
public:
    ~FilePacketizer() = default;

    // Creates an instance of the class.
    // Parameter |file_path| contains the full path to the file.
    // If the specified file can not be opened for reading, then returns nullptr.
    static std::unique_ptr<FilePacketizer> create(const std::filesystem::path& file_path);

    // Creates a packet for transferring.
    std::unique_ptr<proto::file_transfer::Packet> readNextPacket(
        const proto::file_transfer::PacketRequest& request);

private:
    FilePacketizer(std::ifstream&& file_stream);

    std::ifstream file_stream_;

    std::streamoff file_size_ = 0;
    std::streamoff left_size_ = 0;

    DISALLOW_COPY_AND_ASSIGN(FilePacketizer);
};

} // namespace aspia

#endif // ASPIA_SHARE__FILE_PACKETIZER_H_