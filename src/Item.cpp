// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "DateTime.h"
#include "Types.h"

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

Item::Item(const std::shared_ptr<const Store>& store, IMessage* pMessage, size_t columnCount,
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

const response::StringType& Item::subject() const
{
	return m_subject;
}

const std::optional<response::StringType>& Item::sender() const
{
	return m_sender;
}

const std::optional<response::StringType>& Item::to() const
{
	return m_to;
}

const std::optional<response::StringType>& Item::cc() const
{
	return m_cc;
}

response::BooleanType Item::read() const
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

const CComPtr<IMessage>& Item::message() const
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

response::StringType Item::GetStringColumn(DefaultColumn column) const
{
	const auto& stringProp = GetColumnProp(column);

	if (PROP_TYPE(stringProp.ulPropTag) != PT_UNICODE)
	{
		return {};
	}

	return convert::utf8::to_utf8(stringProp.Value.lpszW);
}

response::BooleanType Item::GetReadColumn(DefaultColumn column) const
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

void Item::OpenItem() const
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

service::FieldResult<response::IdType> Item::getId(service::FieldParams&& params) const
{
	return m_id;
}

service::FieldResult<std::shared_ptr<object::Folder>> Item::getParentFolder(
	service::FieldParams&& params) const
{
	return m_store.lock()->OpenFolder(m_parentId);
}

service::FieldResult<std::shared_ptr<object::Conversation>> Item::getConversation(
	service::FieldParams&& params) const
{
	// NYI
	return std::shared_ptr<object::Conversation> {};
}

service::FieldResult<response::StringType> Item::getSubject(service::FieldParams&& params) const
{
	return m_subject;
}

service::FieldResult<std::optional<response::StringType>> Item::getSender(
	service::FieldParams&& params) const
{
	return m_sender;
}

service::FieldResult<std::optional<response::StringType>> Item::getTo(
	service::FieldParams&& params) const
{
	return m_to;
}

service::FieldResult<std::optional<response::StringType>> Item::getCc(
	service::FieldParams&& params) const
{
	return m_cc;
}

service::FieldResult<std::optional<response::Value>> Item::getBody(
	service::FieldParams&& params) const
{
	// NYI
	return std::nullopt;
}

service::FieldResult<response::BooleanType> Item::getRead(service::FieldParams&& params) const
{
	return m_read;
}

service::FieldResult<std::optional<response::Value>> Item::getReceived(
	service::FieldParams&& params) const
{
	return std::make_optional<response::Value>(convert::datetime::to_string(m_received));
}

service::FieldResult<std::optional<response::Value>> Item::getModified(
	service::FieldParams&& params) const
{
	return std::make_optional<response::Value>(convert::datetime::to_string(m_modified));
}

service::FieldResult<std::optional<response::StringType>> Item::getPreview(
	service::FieldParams&& params) const
{
	return m_preview;
}

service::FieldResult<std::vector<std::shared_ptr<object::Property>>> Item::getColumns(
	service::FieldParams&& params) const
{
	auto store = m_store.lock();
	const auto offset = static_cast<size_t>(DefaultColumn::Count);

	CFRt(m_columnCount >= offset);
	return { store->GetColumns(m_columnCount - offset, m_columns.get() + offset) };
}

service::FieldResult<std::vector<std::shared_ptr<service::Object>>> Item::getAttachments(
	service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg) const
{
	// NYI
	return std::vector<std::shared_ptr<service::Object>> {};
}

} // namespace graphql::mapi