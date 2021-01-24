// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Types.h"
#include "Guid.h"

namespace graphql::mapi {

constexpr ULONG GetColumnPropType(Folder::DefaultColumn column)
{
	return PROP_TYPE(Folder::GetFolderColumns()[static_cast<size_t>(column)]);
}

static_assert(GetColumnPropType(Folder::DefaultColumn::InstanceKey) == PT_BINARY, "type mismatch");
static_assert(GetColumnPropType(Folder::DefaultColumn::Id) == PT_BINARY, "type mismatch");
static_assert(GetColumnPropType(Folder::DefaultColumn::ParentId) == PT_BINARY, "type mismatch");
static_assert(GetColumnPropType(Folder::DefaultColumn::Name) == PT_UNICODE, "type mismatch");
static_assert(GetColumnPropType(Folder::DefaultColumn::Total) == PT_LONG, "type mismatch");
static_assert(GetColumnPropType(Folder::DefaultColumn::Unread) == PT_LONG, "type mismatch");

Folder::Folder(const std::shared_ptr<const Store>& store, IMAPIFolder* pFolder, size_t columnCount,
	mapi_ptr<SPropValue>&& columns)
	: m_store { store }
	, m_columnCount { columnCount }
	, m_columns { std::move(columns) }
	, m_instanceKey { GetIdColumn(DefaultColumn::InstanceKey) }
	, m_id { GetIdColumn(DefaultColumn::Id) }
	, m_parentId { GetIdColumn(DefaultColumn::ParentId) }
	, m_name { GetStringColumn(DefaultColumn::Name) }
	, m_count { GetIntColumn(DefaultColumn::Total) }
	, m_unread { GetIntColumn(DefaultColumn::Unread) }
	, m_specialFolder { [this]() {
		// Check if this folder matches a special folder, and re-use the special folder if it does.
		auto spStore = m_store.lock();
		const auto& specialFolders = spStore->specialFolders();
		auto itr = std::find_if(specialFolders.begin(),
			specialFolders.end(),
			[this, &spStore](const auto& entry) noexcept {
				ULONG result = 0;

				return SUCCEEDED(spStore->store()->CompareEntryIDs(static_cast<ULONG>(id().size()),
						   reinterpret_cast<LPENTRYID>(const_cast<response::IdType&>(id()).data()),
						   static_cast<ULONG>(entry.second.size()),
						   reinterpret_cast<LPENTRYID>(
							   const_cast<response::IdType&>(entry.second).data()),
						   0,
						   &result))
					&& result != 0;
			});

		return itr == specialFolders.end() ? std::nullopt : std::make_optional(itr->first);
	}() }
	, m_folder { pFolder }
{
}

Folder::~Folder()
{
	// Release all of the MAPI objects we opened and free any MAPI memory allocations.
	if (m_subFolderSink)
	{
		m_subFolderSink->Unadvise();
	}

	if (m_itemSink)
	{
		m_itemSink.Release();
	}
}

const response::IdType& Folder::instanceKey() const
{
	return m_instanceKey;
}

const response::IdType& Folder::id() const
{
	return m_id;
}

const response::StringType& Folder::name() const
{
	return m_name;
}

response::IntType Folder::count() const
{
	return m_count;
}

response::IntType Folder::unread() const
{
	return m_unread;
}

const CComPtr<IMAPIFolder>& Folder::folder() const
{
	OpenFolder();
	return m_folder;
}

const std::vector<std::shared_ptr<Folder>>& Folder::subFolders() const
{
	LoadSubFolders({});
	return *m_subFolders;
}

std::shared_ptr<Folder> Folder::parentFolder() const
{
	return m_parentId == m_id ? nullptr : m_store.lock()->OpenFolder(m_parentId);
}

std::shared_ptr<Folder> Folder::lookupSubFolder(const response::IdType& id) const
{
	LoadSubFolders({});

	auto itr = m_subFolderIds->find(id);

	if (itr == m_subFolderIds->cend())
	{
		return nullptr;
	}

	return m_subFolders->at(itr->second);
}

const std::vector<std::shared_ptr<Item>>& Folder::items() const
{
	LoadItems({});
	return *m_items;
}

std::shared_ptr<Item> Folder::lookupItem(const response::IdType& id) const
{
	LoadItems({});

	auto itr = m_itemIds->find(id);

	if (itr == m_itemIds->cend())
	{
		return nullptr;
	}

	return m_items->at(itr->second);
}

const SPropValue& Folder::GetColumnProp(DefaultColumn column) const
{
	const auto index = static_cast<size_t>(column);

	CFRt(m_columnCount > static_cast<size_t>(index));

	return m_columns.get()[index];
}

response::IdType Folder::GetIdColumn(DefaultColumn column) const
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

response::StringType Folder::GetStringColumn(DefaultColumn column) const
{
	const auto& stringProp = GetColumnProp(column);

	if (PROP_TYPE(stringProp.ulPropTag) != PT_UNICODE)
	{
		return {};
	}

	return convert::utf8::to_utf8(stringProp.Value.lpszW);
}

response::IntType Folder::GetIntColumn(DefaultColumn column) const
{
	const auto& intProp = GetColumnProp(column);

	if (PROP_TYPE(intProp.ulPropTag) != PT_LONG)
	{
		return 0;
	}

	return static_cast<response::IntType>(intProp.Value.l);
}

void Folder::OpenFolder() const
{
	if (m_folder)
	{
		return;
	}

	ULONG objType = 0;

	CORt(m_store.lock()->store()->OpenEntry(static_cast<ULONG>(m_id.size()),
		reinterpret_cast<LPENTRYID>(const_cast<response::IdType&>(m_id).data()),
		&IID_IMAPIFolder,
		MAPI_BEST_ACCESS | MAPI_DEFERRED_ERRORS,
		&objType,
		reinterpret_cast<LPUNKNOWN*>(&m_folder)));
	CFRt(m_folder != nullptr);
	CFRt(objType == MAPI_FOLDER);
}

void Folder::LoadSubFolders(response::Value&& fieldDirectives) const
{
	if (m_subFolderDirectives != fieldDirectives)
	{
		// Reset the subFolders and reload if the directives change
		m_subFolders.reset();
		m_subFolderDirectives = std::move(fieldDirectives);
	}

	if (m_subFolders)
	{
		return;
	}

	m_subFolderIds = std::make_unique<std::map<response::IdType, size_t>>();
	m_subFolders = std::make_unique<std::vector<std::shared_ptr<Folder>>>();

	constexpr auto c_folderProps = GetFolderColumns();
	mapi_ptr<SPropTagArray> folderProps;

	CORt(::MAPIAllocateBuffer(CbNewSPropTagArray(c_folderProps.size()),
		reinterpret_cast<void**>(&out_ptr { folderProps })));
	CFRt(folderProps != nullptr);
	folderProps->cValues = static_cast<ULONG>(c_folderProps.size());
	std::copy(c_folderProps.begin(), c_folderProps.end(), folderProps->aulPropTag);

	constexpr auto c_folderSorts = GetFolderSorts();
	mapi_ptr<SSortOrderSet> folderSorts;

	CORt(::MAPIAllocateBuffer(CbNewSSortOrderSet(c_folderSorts.size()),
		reinterpret_cast<void**>(&out_ptr { folderSorts })));
	CFRt(folderSorts != nullptr);
	folderSorts->cSorts = static_cast<ULONG>(c_folderSorts.size());
	folderSorts->cCategories = 0;
	folderSorts->cExpanded = 0;
	std::copy(c_folderSorts.begin(), c_folderSorts.end(), folderSorts->aSort);

	auto store = m_store.lock();
	const TableDirectives directives { store, m_subFolderDirectives };
	CComPtr<IMAPITable> sptable;

	CORt(folder()->GetHierarchyTable(MAPI_DEFERRED_ERRORS | MAPI_UNICODE, &sptable));

	const rowset_ptr sprows =
		directives.read(sptable, std::move(folderProps), std::move(folderSorts));

	m_subFolders->reserve(static_cast<size_t>(sprows->cRows));
	for (ULONG i = 0; i != sprows->cRows; i++)
	{
		auto& row = sprows->aRow[i];
		const size_t columnCount = static_cast<size_t>(row.cValues);
		mapi_ptr<SPropValue> columns { row.lpProps };

		row.lpProps = nullptr;

		auto folder = std::make_shared<Folder>(store, nullptr, columnCount, std::move(columns));

		store->CacheFolder(folder);
		m_subFolderIds->insert(std::make_pair(folder->id(), m_subFolders->size()));
		m_subFolders->push_back(std::move(folder));
	}

	if (!m_subFolderSink)
	{
		auto spThis = std::static_pointer_cast<const Folder>(shared_from_this());
		CComPtr<AdviseSinkProxy<IMAPITable>> sinkProxy;
		ULONG_PTR connectionId = 0;

		sinkProxy.Attach(new AdviseSinkProxy<IMAPITable>(
			[wpFolder = std::weak_ptr { spThis }](size_t, LPNOTIFICATION) {
				auto spFolder = wpFolder.lock();

				if (spFolder)
				{
					spFolder->m_subFolders.reset();
				}
			}));

		CORt(sptable->Advise(fnevTableModified, sinkProxy, &connectionId));
		sinkProxy->OnAdvise(sptable, connectionId);

		m_subFolderSink = sinkProxy;
	}
}

void Folder::LoadItems(response::Value&& fieldDirectives) const
{
	if (m_itemDirectives != fieldDirectives)
	{
		// Reset the items and reload if the directives change
		m_items.reset();
		m_itemDirectives = std::move(fieldDirectives);
	}

	if (m_items)
	{
		return;
	}

	m_itemIds = std::make_unique<std::map<response::IdType, size_t>>();
	m_items = std::make_unique<std::vector<std::shared_ptr<Item>>>();

	constexpr auto c_itemProps = Item::GetItemColumns();
	mapi_ptr<SPropTagArray> itemProps;

	CORt(::MAPIAllocateBuffer(CbNewSPropTagArray(c_itemProps.size()),
		reinterpret_cast<void**>(&out_ptr { itemProps })));
	CFRt(itemProps != nullptr);
	itemProps->cValues = static_cast<ULONG>(c_itemProps.size());
	std::copy(c_itemProps.begin(), c_itemProps.end(), itemProps->aulPropTag);

	constexpr auto c_itemSorts = Item::GetItemSorts();
	mapi_ptr<SSortOrderSet> itemSorts;

	CORt(::MAPIAllocateBuffer(CbNewSSortOrderSet(c_itemSorts.size()),
		reinterpret_cast<void**>(&out_ptr { itemSorts })));
	CFRt(itemSorts != nullptr);
	itemSorts->cSorts = static_cast<ULONG>(c_itemSorts.size());
	itemSorts->cCategories = 0;
	itemSorts->cExpanded = 0;
	std::copy(c_itemSorts.begin(), c_itemSorts.end(), itemSorts->aSort);

	auto store = m_store.lock();
	const TableDirectives directives { store, m_itemDirectives };
	CComPtr<IMAPITable> sptable;

	CORt(folder()->GetContentsTable(MAPI_DEFERRED_ERRORS | MAPI_UNICODE, &sptable));

	const rowset_ptr sprows = directives.read(sptable, std::move(itemProps), std::move(itemSorts));

	m_items->reserve(static_cast<size_t>(sprows->cRows));
	for (ULONG i = 0; i != sprows->cRows; i++)
	{
		auto& row = sprows->aRow[i];
		const size_t columnCount = static_cast<size_t>(row.cValues);
		mapi_ptr<SPropValue> columns { row.lpProps };

		row.lpProps = nullptr;

		auto item = std::make_shared<Item>(store, nullptr, columnCount, std::move(columns));

		store->CacheItem(item);
		m_itemIds->insert({ item->id(), m_items->size() });
		m_items->push_back(std::move(item));
	}

	if (!m_itemSink)
	{
		auto spThis = std::static_pointer_cast<const Folder>(shared_from_this());
		CComPtr<AdviseSinkProxy<IMAPITable>> sinkProxy;
		ULONG_PTR connectionId = 0;

		sinkProxy.Attach(new AdviseSinkProxy<IMAPITable>(
			[wpFolder = std::weak_ptr { spThis }](size_t, LPNOTIFICATION) {
				auto spFolder = wpFolder.lock();

				if (spFolder)
				{
					spFolder->m_items.reset();
				}
			}));

		CORt(sptable->Advise(fnevTableModified, sinkProxy, &connectionId));
		sinkProxy->OnAdvise(sptable, connectionId);

		m_itemSink = sinkProxy;
	}
}

service::FieldResult<response::IdType> Folder::getId(service::FieldParams&& params) const
{
	return { m_id };
}

service::FieldResult<std::shared_ptr<object::Folder>>
Folder::getParentFolder(service::FieldParams&& params) const
{
	return parentFolder();
}

service::FieldResult<std::shared_ptr<object::Store>>
Folder::getStore(service::FieldParams&& params) const
{
	return std::static_pointer_cast<object::Store>(std::const_pointer_cast<Store>(m_store.lock()));
}

service::FieldResult<response::StringType> Folder::getName(service::FieldParams&& params) const
{
	return { m_name };
}

service::FieldResult<response::IntType> Folder::getCount(service::FieldParams&& params) const
{
	return { m_count };
}

service::FieldResult<response::IntType> Folder::getUnread(service::FieldParams&& params) const
{
	return { m_unread };
}

service::FieldResult<std::optional<SpecialFolder>>
Folder::getSpecialFolder(service::FieldParams&& params) const
{
	return { m_specialFolder };
}

service::FieldResult<std::vector<std::shared_ptr<object::Property>>>
Folder::getColumns(service::FieldParams&& params) const
{
	auto store = m_store.lock();
	const auto offset = static_cast<size_t>(DefaultColumn::Count);

	CFRt(m_columnCount >= offset);
	return { store->GetColumns(m_columnCount - offset, m_columns.get() + offset) };
}

service::FieldResult<std::vector<std::shared_ptr<object::Folder>>> Folder::getSubFolders(
	service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg) const
{
	std::vector<std::shared_ptr<object::Folder>> result {};

	LoadSubFolders(std::move(params.fieldDirectives));

	if (idsArg)
	{
		// Lookup the folders with the specified IDs
		result.resize(idsArg->size());
		std::transform(idsArg->cbegin(),
			idsArg->cend(),
			result.begin(),
			[this](const response::IdType& id) noexcept {
				return std::static_pointer_cast<object::Folder>(lookupSubFolder(id));
			});
	}
	else
	{
		// Just clone the entire vector of folders.
		result.resize(m_subFolders->size());
		std::transform(m_subFolders->cbegin(),
			m_subFolders->cend(),
			result.begin(),
			[this](const std::shared_ptr<Folder>& folder) noexcept {
				return std::static_pointer_cast<object::Folder>(folder);
			});
	}

	return result;
}

service::FieldResult<std::vector<std::shared_ptr<object::Conversation>>> Folder::getConversations(
	service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg) const
{
	// NYI
	return std::vector<std::shared_ptr<object::Conversation>> {};
}

service::FieldResult<std::vector<std::shared_ptr<object::Item>>> Folder::getItems(
	service::FieldParams&& params, std::optional<std::vector<response::IdType>>&& idsArg) const
{
	std::vector<std::shared_ptr<object::Item>> result {};

	LoadItems(std::move(params.fieldDirectives));

	if (idsArg)
	{
		// Lookup the items with the specified IDs
		result.resize(idsArg->size());
		std::transform(idsArg->cbegin(),
			idsArg->cend(),
			result.begin(),
			[this](const response::IdType& id) noexcept {
				return std::static_pointer_cast<object::Item>(lookupItem(id));
			});
	}
	else
	{
		// Just clone the entire vector of items.
		result.resize(m_items->size());
		std::transform(m_items->cbegin(),
			m_items->cend(),
			result.begin(),
			[this](const std::shared_ptr<Item>& item) noexcept {
				return std::static_pointer_cast<object::Item>(item);
			});
	}

	return result;
}

} // namespace graphql::mapi