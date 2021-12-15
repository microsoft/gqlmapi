// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "DateTime.h"
#include "Types.h"

#include "AttachmentObject.h"
#include "ConversationObject.h"
#include "FileAttachmentObject.h"
#include "FolderObject.h"

namespace graphql::mapi {

constexpr ULONG GetColumnPropType(Item::DefaultColumn column)
{
	return PROP_TYPE(Item::GetItemColumns()[static_cast<size_t>(column)]);
}

static_assert(GetColumnPropType(Item::DefaultColumn::InstanceKey) == PT_BINARY, "type mismatch");
static_assert(GetColumnPropType(Item::DefaultColumn::Id) == PT_BINARY, "type mismatch");
static_assert(GetColumnPropType(Item::DefaultColumn::ParentId) == PT_BINARY, "type mismatch");
static_assert(GetColumnPropType(Item::DefaultColumn::Subject) == PT_UNICODE, "type mismatch");
static_assert(GetColumnPropType(Item::DefaultColumn::Sender) == PT_UNICODE, "type mismatch");
static_assert(GetColumnPropType(Item::DefaultColumn::To) == PT_UNICODE, "type mismatch");
static_assert(GetColumnPropType(Item::DefaultColumn::Cc) == PT_UNICODE, "type mismatch");
static_assert(GetColumnPropType(Item::DefaultColumn::MessageFlags) == PT_LONG, "type mismatch");
static_assert(GetColumnPropType(Item::DefaultColumn::Received) == PT_SYSTIME, "type mismatch");
static_assert(GetColumnPropType(Item::DefaultColumn::Modified) == PT_SYSTIME, "type mismatch");
static_assert(GetColumnPropType(Item::DefaultColumn::Preview) == PT_UNICODE, "type mismatch");

Item::Item(const std::shared_ptr<Store>& store, IMessage* pMessage, size_t columnCount,
	mapi_ptr<SPropValue>&& columns)
	: m_store { store }
	, m_columnCount { columnCount }
	, m_columns { std::move(columns) }
	, m_instanceKey { GetIdColumn(DefaultColumn::InstanceKey) }
	, m_id { GetIdColumn(DefaultColumn::Id) }
	, m_parentId { GetIdColumn(DefaultColumn::ParentId) }
	, m_subject { GetStringColumn(DefaultColumn::Subject) }
	, m_sender { GetStringColumn(DefaultColumn::Sender) }
	, m_to { GetStringColumn(DefaultColumn::To) }
	, m_cc { GetStringColumn(DefaultColumn::Cc) }
	, m_read { GetReadColumn(DefaultColumn::MessageFlags) }
	, m_received { GetTimeColumn(DefaultColumn::Received) }
	, m_modified { GetTimeColumn(DefaultColumn::Modified) }
	, m_preview { GetStringColumn(DefaultColumn::Preview) }
	, m_message { pMessage }
{
}

const response::IdType& Item::instanceKey() const
{
	return m_instanceKey;
}

const response::IdType& Item::id() const
{
	return m_id;
}

const std::string& Item::subject() const
{
	return m_subject;
}

const std::optional<std::string>& Item::sender() const
{
	return m_sender;
}

const std::optional<std::string>& Item::to() const
{
	return m_to;
}

const std::optional<std::string>& Item::cc() const
{
	return m_cc;
}

bool Item::read() const
{
	return m_read;
}

const FILETIME& Item::received() const
{
	return m_received;
}

const FILETIME& Item::modified() const
{
	return m_modified;
}

const CComPtr<IMessage>& Item::message()
{
	OpenItem();
	return m_message;
}

const SPropValue& Item::GetColumnProp(DefaultColumn column) const
{
	const auto index = static_cast<size_t>(column);

	CFRt(m_columnCount > static_cast<size_t>(index));

	return m_columns.get()[index];
}

response::IdType Item::GetIdColumn(DefaultColumn column) const
{
	const auto& idProp = GetColumnProp(column);

	if (PROP_TYPE(idProp.ulPropTag) != PT_BINARY)
	{
		return {};
	}

	const auto idBegin = reinterpret_cast<std::uint8_t*>(idProp.Value.bin.lpb);
	const auto idEnd = idBegin + idProp.Value.bin.cb;

	return { idBegin, idEnd };
}

std::string Item::GetStringColumn(DefaultColumn column) const
{
	const auto& stringProp = GetColumnProp(column);

	if (PROP_TYPE(stringProp.ulPropTag) != PT_UNICODE)
	{
		return {};
	}

	return convert::utf8::to_utf8(stringProp.Value.lpszW);
}

bool Item::GetReadColumn(DefaultColumn column) const
{
	const auto& messageFlagsProp = GetColumnProp(column);

	return (PROP_TYPE(messageFlagsProp.ulPropTag) == PT_LONG)
		&& ((messageFlagsProp.Value.l & MSGFLAG_READ) == MSGFLAG_READ);
}

FILETIME Item::GetTimeColumn(DefaultColumn column) const
{
	const auto& timeProp = GetColumnProp(column);

	CFRt(PROP_TYPE(timeProp.ulPropTag) == PT_SYSTIME);

	return timeProp.Value.ft;
}

void Item::OpenItem()
{
	if (m_message)
	{
		return;
	}

	ULONG objType = 0;

	CORt(m_store.lock()->store()->OpenEntry(static_cast<ULONG>(m_id.size()),
		reinterpret_cast<LPENTRYID>(const_cast<response::IdType&>(m_id).data()),
		&IID_IMessage,
		MAPI_BEST_ACCESS | MAPI_DEFERRED_ERRORS,
		&objType,
		reinterpret_cast<LPUNKNOWN*>(&m_message)));
	CFRt(m_message != nullptr);
	CFRt(objType == MAPI_MESSAGE);
}

const response::IdType& Item::getId() const
{
	return m_id;
}

std::shared_ptr<object::Folder> Item::getParentFolder() const
{
	return std::make_shared<object::Folder>(m_store.lock()->OpenFolder(m_parentId));
}

std::shared_ptr<object::Conversation> Item::getConversation(service::FieldParams&& params) const
{
	// NYI
	return {};
}

const std::string& Item::getSubject() const
{
	return m_subject;
}

std::optional<std::string> Item::getSender() const
{
	return m_sender;
}

std::optional<std::string> Item::getTo() const
{
	return m_to;
}

std::optional<std::string> Item::getCc() const
{
	return m_cc;
}

std::optional<response::Value> Item::getBody(service::FieldParams&& params) const
{
	// NYI
	return std::nullopt;
}

bool Item::getRead() const
{
	return m_read;
}

std::optional<response::Value> Item::getReceived() const
{
	return std::make_optional<response::Value>(convert::datetime::to_string(m_received));
}

std::optional<response::Value> Item::getModified() const
{
	return std::make_optional<response::Value>(convert::datetime::to_string(m_modified));
}

std::optional<std::string> Item::getPreview() const
{
	return m_preview;
}

std::vector<std::shared_ptr<object::Property>> Item::getColumns() const
{
	auto store = m_store.lock();
	const auto offset = static_cast<size_t>(DefaultColumn::Count);

	CFRt(m_columnCount >= offset);
	return { store->GetColumns(m_columnCount - offset, m_columns.get() + offset) };
}

std::vector<std::shared_ptr<object::Attachment>> Item::getAttachments(
	service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg) const
{
	// NYI
	return {};
}

} // namespace graphql::mapi