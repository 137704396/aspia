//
// Aspia Project
// Copyright (C) 2020 Dmitry Chapyshev <dmitry@aspia.ru>
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

#ifndef NET__PEER_CONTROLLER_H
#define NET__PEER_CONTROLLER_H

#include "base/macros_magic.h"
#include "net/channel.h"

#include <memory>
#include <string>

namespace net {

class PeerController : public Channel::Listener
{
public:
    PeerController();
    ~PeerController();

    struct Router
    {
        std::u16string address;
        uint16_t port;
        base::ByteArray public_key;
        std::u16string username;
        std::u16string password;
    };

    using RouterList = std::vector<Router>;

    class Delegate
    {
    public:
        virtual ~Delegate() = default;

        virtual void onRouterConnected() = 0;
        virtual void onRouterDisconnected() = 0;
        virtual void onPeerConnected(std::unique_ptr<Channel> channel) = 0;
    };

    void start(const RouterList& router_list, Delegate* delegate);
    void connectTo(uint64_t peer_id);

protected:
    // Channel::Listener implementation.
    void onConnected() override;
    void onDisconnected(Channel::ErrorCode error_code) override;
    void onMessageReceived(const base::ByteArray& buffer) override;
    void onMessageWritten(size_t pending) override;

private:
    void connectToRouter();

    RouterList router_list_;
    Delegate* delegate_ = nullptr;

    size_t current_router_ = 0;
    std::unique_ptr<Channel> channel_;

    DISALLOW_COPY_AND_ASSIGN(PeerController);
};

} // namespace net

#endif // NET__PEER_CONTROLLER_H
