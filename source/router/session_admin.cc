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

#include "router/session_admin.h"

#include "base/logging.h"
#include "base/strings/unicode.h"
#include "base/net/network_channel.h"
#include "base/peer/user.h"
#include "router/database.h"
#include "router/server_proxy.h"

namespace router {

SessionAdmin::SessionAdmin(std::unique_ptr<base::NetworkChannel> channel,
                               std::shared_ptr<DatabaseFactory> database_factory,
                               std::shared_ptr<ServerProxy> server_proxy)
    : Session(proto::ROUTER_SESSION_ADMIN, std::move(channel), std::move(database_factory)),
      server_proxy_(std::move(server_proxy))
{
    DCHECK(server_proxy_);
}

SessionAdmin::~SessionAdmin() = default;

void SessionAdmin::onSessionReady()
{
    // Nothing
}

void SessionAdmin::onMessageReceived(const base::ByteArray& buffer)
{
    proto::AdminToRouter message;

    if (!base::parse(buffer, &message))
    {
        LOG(LS_ERROR) << "Could not read message from manager";
        return;
    }

    if (message.has_host_list_request())
    {
        doPeerListRequest();
    }
    else if (message.has_host_request())
    {
        LOG(LS_INFO) << "PEER REQUEST";
    }
    else if (message.has_relay_list_request())
    {
        doRelayListRequest();
    }
    else if (message.has_user_list_request())
    {
        doUserListRequest();
    }
    else if (message.has_user_request())
    {
        doUserRequest(message.user_request());
    }
    else
    {
        LOG(LS_WARNING) << "Unhandled message from manager";
    }
}

void SessionAdmin::onMessageWritten(size_t pending)
{
    // Nothing
}

void SessionAdmin::doUserListRequest()
{
    std::unique_ptr<Database> database = openDatabase();
    if (!database)
    {
        LOG(LS_ERROR) << "Failed to connect to database";
        return;
    }

    proto::RouterToAdmin message;
    proto::UserList* list = message.mutable_user_list();

    base::UserList users = database->userList();
    for (base::UserList::Iterator it(users); !it.isAtEnd(); it.advance())
        list->add_user()->CopyFrom(it.user().serialize());

    sendMessage(message);
}

void SessionAdmin::doUserRequest(const proto::UserRequest& request)
{
    proto::RouterToAdmin message;
    proto::UserResult* result = message.mutable_user_result();
    result->set_type(request.type());

    switch (request.type())
    {
        case proto::USER_REQUEST_ADD:
            result->set_error_code(addUser(request.user()));
            break;

        case proto::USER_REQUEST_MODIFY:
            result->set_error_code(modifyUser(request.user()));
            break;

        case proto::USER_REQUEST_DELETE:
            result->set_error_code(deleteUser(request.user()));
            break;
    }

    sendMessage(message);
}

void SessionAdmin::doRelayListRequest()
{
    proto::RouterToAdmin message;

    message.set_allocated_relay_list(server_proxy_->relayList().release());
    if (!message.has_relay_list())
        message.mutable_relay_list()->set_error_code(proto::RelayList::UNKNOWN_ERROR);

    sendMessage(message);
}

void SessionAdmin::doPeerListRequest()
{
    proto::RouterToAdmin message;

    message.set_allocated_host_list(server_proxy_->hostList().release());
    if (!message.has_host_list())
        message.mutable_host_list()->set_error_code(proto::HostList::UNKNOWN_ERROR);

    sendMessage(message);
}

proto::UserResult::ErrorCode SessionAdmin::addUser(const proto::User& user)
{
    LOG(LS_INFO) << "User add request: " << user.name();

    base::User new_user = base::User::parseFrom(user);
    if (!new_user.isValid())
    {
        LOG(LS_ERROR) << "Failed to create user";
        return proto::UserResult::INTERNAL_ERROR;
    }

    std::unique_ptr<Database> database = openDatabase();
    if (!database)
    {
        LOG(LS_ERROR) << "Failed to connect to database";
        return proto::UserResult::INTERNAL_ERROR;
    }

    if (!database->addUser(new_user))
        return proto::UserResult::INTERNAL_ERROR;

    return proto::UserResult::SUCCESS;
}

proto::UserResult::ErrorCode SessionAdmin::modifyUser(const proto::User& user)
{
    LOG(LS_INFO) << "User modify request: " << user.name();

    if (user.entry_id() <= 0)
    {
        LOG(LS_ERROR) << "Invalid user ID: " << user.entry_id();
        return proto::UserResult::INVALID_DATA;
    }

    base::User new_user = base::User::parseFrom(user);
    if (!new_user.isValid())
    {
        LOG(LS_ERROR) << "Failed to create user";
        return proto::UserResult::INTERNAL_ERROR;
    }

    std::unique_ptr<Database> database = openDatabase();
    if (!database)
    {
        LOG(LS_ERROR) << "Failed to connect to database";
        return proto::UserResult::INTERNAL_ERROR;
    }

    if (!database->modifyUser(new_user))
        return proto::UserResult::INTERNAL_ERROR;

    return proto::UserResult::SUCCESS;
}

proto::UserResult::ErrorCode SessionAdmin::deleteUser(const proto::User& user)
{
    std::unique_ptr<Database> database = openDatabase();
    if (!database)
    {
        LOG(LS_ERROR) << "Failed to connect to database";
        return proto::UserResult::INTERNAL_ERROR;
    }

    uint64_t entry_id = user.entry_id();

    LOG(LS_INFO) << "User remove request: " << entry_id;

    if (!database->removeUser(entry_id))
        return proto::UserResult::INTERNAL_ERROR;

    return proto::UserResult::SUCCESS;
}

} // namespace router
